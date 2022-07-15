module N64;

import AI;
import Cartridge;
import Input;
import Memory;
import MI;
import PI;
import PIF;
import Renderer;
import RI;
import RSP;
import SI;
import VI;
import VR4300;

namespace N64
{
	void CheckEventQueue()
	{
		for (auto it = event_queue.begin(); it != event_queue.end(); ) {
			EventItem& item = *it;
			item.time -= cpu_cycles_until_update_queue;
			if (item.time <= 0) {
				ExecuteEvent(item.event);
				it = event_queue.erase(it);
			}
			else {
				++it;
			}
		}
	}


	void EnqueueEvent(Event event, uint cycles_until_fire)
	{
		uint cycles_into_update = VR4300::p_cycle_counter;
		cpu_cycles_until_update_queue -= cycles_into_update;

		EventItem new_item = { event, cycles_until_fire };
		bool item_inserted = false;

		for (auto it = event_queue.begin(); it != event_queue.end(); it++) {
			EventItem& item = *it;
			item.time -= cycles_into_update;
			if (item.time <= 0) {
				ExecuteEvent(item.event);
				it = event_queue.erase(it);
				if (it == event_queue.end())
					break;
			}
			else if (!item_inserted && item.time > new_item.time) {
				it = event_queue.insert(it, new_item);
				item_inserted = true;
			}
		}
		if (!item_inserted) {
			event_queue.push_back(new_item);
		}
	}


	void ExecuteEvent(Event event)
	{
		switch (event) {
		case Event::PiDmaFinish:
			MI::SetInterruptFlag(MI::InterruptType::PI);
			PI::SetStatusFlag(PI::StatusFlag::DmaCompleted);
			PI::ClearStatusFlag(PI::StatusFlag::DmaBusy);
			VR4300::CheckInterrupts();
			break;

		case Event::SiDmaFinish:
			MI::SetInterruptFlag(MI::InterruptType::SI);
			SI::SetStatusFlag(SI::StatusFlag::Interrupt);
			SI::ClearStatusFlag(SI::StatusFlag::DmaBusy);
			VR4300::CheckInterrupts();
			break;

		case Event::SpDmaFinish:
			MI::SetInterruptFlag(MI::InterruptType::SP);
			RSP::NotifyDmaFinish();
			VR4300::CheckInterrupts();
			break;
		}
	}


	bool PowerOn(
		const std::string& rom_path,
		const std::optional<std::string>& ipl_path,
		SDL_Renderer* renderer,
		uint window_width,
		uint window_height)
	{
		bool success = Cartridge::LoadROM(rom_path);
		if (!success) {
			return false;
		}

		bool hle_ipl = true;
		if (ipl_path.has_value()) {
			success = PIF::LoadIPL12(ipl_path.value());
			hle_ipl = !success;
		}

		AI::Initialize();
		MI::Initialize();
		PI::Initialize();
		RI::Initialize();
		SI::Initialize();
		VI::Initialize();
		Memory::Initialize();
		/* Power CPU after RSP, since CPU reads to RSP memory if hle_ipl and RSP clears it. */
		RSP::PowerOn();
		VR4300::PowerOn(hle_ipl);

		Renderer::Initialize(renderer);
		Renderer::SetWindowSize(window_width, window_height);

		return true;
	}


	void Run()
	{
		while (true) {
			for (uint field = 0; field < VI::num_fields; ++field) {
				uint cpu_cycles_taken_this_frame = 0;
				for (uint line = 0; line < VI::num_halflines; ++line) {
					VI::SetCurrentHalfline((line << 1) + field);
					/* If num_halflines == 262, the number of cycles to update becomes 5963. */
					cpu_cycles_until_update_queue = VI::cpu_cycles_per_halfline;
					VR4300::Run(VI::cpu_cycles_per_halfline);
					RSP::Run(VI::cpu_cycles_per_halfline);
					AI::Step(VI::cpu_cycles_per_halfline);
					CheckEventQueue();
					cpu_cycles_taken_this_frame += VI::cpu_cycles_per_halfline;
				}
				/* Run a few extra cycles if cpu_cycles_per_frame % num_halflines != 0 (quotient is equal to cycles_per_halfline). */
				if (cpu_cycles_taken_this_frame < cpu_cycles_per_frame) {
					uint extra_cycles = cpu_cycles_per_frame - cpu_cycles_taken_this_frame;
					cpu_cycles_until_update_queue = extra_cycles;
					VR4300::Run(extra_cycles);
					RSP::Run(extra_cycles);
					AI::Step(extra_cycles);
					CheckEventQueue();
				}
				Renderer::Render();
				Input::Poll();

				auto time_elapsed = std::chrono::steady_clock::now() - prev_time;
				if (time_elapsed >= std::chrono::milliseconds(1000)) {
					std::cout << frame_counter << " fps" << std::endl;
					prev_time = std::chrono::steady_clock::now();
					frame_counter = 0;
				}
				else {
					frame_counter++;
				}
			}
		}
	}
}