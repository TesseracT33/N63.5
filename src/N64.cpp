module N64;

import Cartridge;
import Input;
import MI;
import PI;
import Renderer;
import VI;
import VR4300;

namespace N64
{
	bool PowerOn(
		const std::string& rom_path,
		SDL_Renderer* renderer,
		const unsigned window_width,
		const unsigned window_height)
	{
		bool success = Cartridge::LoadROM(rom_path);
		if (!success)
			return false;

		MI::Initialize();
		VI::Initialize();
		VR4300::PowerOn(true);

		Renderer::Initialize(renderer);
		Renderer::SetWindowSize(window_width, window_height);

		return true;
	}


	void Run()
	{
		static constexpr int cpu_cycles_per_second = 93'750'000;
		static constexpr int cpu_cycles_per_frame = cpu_cycles_per_second / 60; /* 1,562,500 */
		static constexpr int event_checks_per_frame = 100;
		static constexpr int cpu_cycles_per_event_check = cpu_cycles_per_frame / event_checks_per_frame; /* 15625 */

		while (true)
		{
			for (int i = 0; i < event_checks_per_frame; ++i)
			{
				cpu_cycles_until_update_queue = cpu_cycles_per_event_check;
				VR4300::Run(cpu_cycles_per_event_check);
				CheckEventQueue();
			}
			Renderer::Render();
			Input::Poll();
		}
	}


	void CheckEventQueue()
	{
		for (auto it = event_queue.begin(); it != event_queue.end(); it++)
		{
			EventItem& item = *it;
			item.time -= cpu_cycles_until_update_queue;
			if (item.time <= 0)
			{
				ExecuteEvent(item.event);
				it = event_queue.erase(it);
				if (it == event_queue.end())
					break;
			}
		}
	}


	void EnqueueEvent(Event event, int cycles_until_fire, int cycles_into_update)
	{
		cpu_cycles_until_update_queue -= cycles_into_update;

		EventItem new_item = { event, cycles_until_fire };
		bool item_inserted = false;

		for (auto it = event_queue.begin(); it != event_queue.end(); it++)
		{
			EventItem& item = *it;
			item.time -= cycles_into_update;
			if (item.time <= 0)
			{
				ExecuteEvent(item.event);
				it = event_queue.erase(it);
				if (it == event_queue.end())
					break;
			}
			else if (!item_inserted && item.time > new_item.time)
			{
				it = event_queue.insert(it, new_item);
				item_inserted = true;
			}
		}
		if (!item_inserted)
		{
			event_queue.push_back(new_item);
		}
	}


	void ExecuteEvent(Event event)
	{
		switch (event)
		{
		case Event::PI_DMA_FINISH:
			MI::SetInterruptFlag<MI::InterruptType::PI>();
			PI::SetStatusFlag<PI::StatusFlag::DMA_COMPLETED>();
			PI::ClearStatusFlag<PI::StatusFlag::DMA_BUSY>();
			VR4300::CheckInterrupts();
			break;
		}
	}
}