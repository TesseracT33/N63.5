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
		while (true)
		{
			for (int field = 0; field < VI::num_fields; ++field)
			{
				int cpu_cycles_taken_this_frame = 0;
				for (int line = 0; line < VI::num_halflines; ++line)
				{
					VI::SetCurrentHalfline((line << 1) + field);
					/* If num_halflines == 262, the number of cycles to update becomes 5963. */
					cpu_cycles_until_update_queue = VI::cpu_cycles_per_halfline;
					VR4300::Run(VI::cpu_cycles_per_halfline);
					CheckEventQueue();
					cpu_cycles_taken_this_frame += VI::cpu_cycles_per_halfline;
				}
				/* Run a few extra cycles if cpu_cycles_per_frame % num_halflines != 0 (quotient is equal to cycles_per_halfline). */
				if (cpu_cycles_taken_this_frame < cpu_cycles_per_frame)
				{
					VR4300::Run(cpu_cycles_per_frame - cpu_cycles_taken_this_frame);
					CheckEventQueue();
				}
				Renderer::Render();
				Input::Poll();
			}
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