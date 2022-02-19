module N64;

import Cartridge;
import MI;
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
		static constexpr int cycles_per_update = 10000;

		while (true)
		{
			VR4300::Run(cycles_per_update);
			Renderer::Render();
		}
	}


	void EnqueueEvent(Event event, int cycles_until_fire, int cycles_into_update)
	{
		EventItem new_item = { event, cycles_until_fire };
		bool item_inserted = false;

		for (auto it = event_queue.begin(); it != event_queue.end(); it++) {
			EventItem& item = *it;
			item.time -= cycles_into_update;
			if (item.time <= 0)
			{
				ExecuteEvent(item.event);
				event_queue.erase(it);
			}
			else if (!item_inserted && item.time > new_item.time)
			{
				event_queue.insert(it, new_item);
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
			VR4300::CheckInterrupts();
			break;
		}
	}
}