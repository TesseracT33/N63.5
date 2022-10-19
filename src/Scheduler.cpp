module Scheduler;

import Input;
import N64;
import Renderer;
import RSP;
import VR4300;

namespace Scheduler
{
	void AddEvent(EventType event_type, Time cpu_cycles_until_fire, EventCallback callback)
	{
		auto cpu_cycles_this_exec = 0;
		Time insert_time = cpu_cycles_until_fire + cpu_cycles_per_update - cpu_cycles_this_exec;
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (cpu_cycles_until_fire < it->time_until_fire - cpu_cycles_this_exec) {
				events.emplace(it, event_type, insert_time, callback);
				return;
			}
		}
		events.emplace_back(event_type, insert_time, callback);
	}


	void ChangeEventTime(EventType event_type, Time cpu_cycles_until_fire)
	{
		auto cpu_cycles_this_exec = 0;
		Time insert_time = cpu_cycles_until_fire + cpu_cycles_per_update - cpu_cycles_this_exec;
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (it->event_type == event_type) {
				EventCallback callback = it->callback;
				events.erase(it);
				AddEvent(event_type, cpu_cycles_until_fire, callback);
				return;
			}
		}
	}


	void CheckEvents()
	{
		label:
		for (auto it = events.begin(); it != events.end(); ) {
			it->time_until_fire -= cpu_cycles_per_update;
			if (it->time_until_fire <= 0) {
				/* erase element before invoking callback, in case it mutates the event list */
				EventCallback callback = it->callback;
				it = events.erase(it);
				callback();
				goto label;
			}
			else {
				++it;
			}
		}
	}


	void Initialize()
	{
		events.clear();
		events.reserve(16);
		VR4300::AddInitialEvents();
		// TODO: remove this in favour of approach used in GBA emu
		AddEvent(EventType::Render, N64::cpu_cycles_per_frame, OnRenderEvent);
	}


	void OnRenderEvent()
	{
		Renderer::Render();
		Input::Poll();
		AddEvent(EventType::Render, N64::cpu_cycles_per_frame, OnRenderEvent);
	}


	void RemoveEvent(EventType event_type)
	{
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (it->event_type == event_type) {
				events.erase(it);
				return;
			}
		}
	}


	void Run()
	{
		Time extra_cpu_cycles = 0, extra_rsp_cycles = 0;
		while (true) {
			extra_cpu_cycles = VR4300::Run(cpu_cycles_per_update - extra_cpu_cycles);
			extra_rsp_cycles = RSP::Run(cpu_cycles_per_update - extra_rsp_cycles);
			CheckEvents();
		}
	}
}