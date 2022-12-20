module Scheduler;

import RSP;
import VI;
import VR4300;

namespace Scheduler
{
	void AddEvent(EventType event_type, s64 cpu_cycles_until_fire, EventCallback callback)
	{
		/* Compensate for the fact that we may be in the middle of a CPU update, and times for other events
			have not updated yet. TODO: We are assuming that only the main CPU can cause an event to be added.
			Is it ok? */
		s64 elapsed_cycles_since_step_start = s64(VR4300::GetElapsedCycles());
		s64 enqueue_time = cpu_cycles_until_fire + elapsed_cycles_since_step_start;
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (enqueue_time < it->cpu_cycles_until_fire) {
				events.emplace(it, event_type, enqueue_time, callback);
				return;
			}
		}
		events.emplace_back(event_type, enqueue_time, callback);
	}


	void ChangeEventTime(EventType event_type, s64 cpu_cycles_until_fire)
	{
		s64 elapsed_cycles_since_step_start = s64(VR4300::GetElapsedCycles());
		s64 enqueue_time = cpu_cycles_until_fire + elapsed_cycles_since_step_start;
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (it->event_type == event_type) {
				EventCallback callback = it->callback;
				events.erase(it);
				AddEvent(event_type, enqueue_time, callback);
				return;
			}
		}
	}


	void CheckEvents(s64 cpu_cycle_step)
	{
		for (auto it = events.begin(); it != events.end(); ) {
			it->cpu_cycles_until_fire -= cpu_cycle_step;
			if (it->cpu_cycles_until_fire <= 0) {
				/* erase element before invoking callback, in case it mutates the event list */
				EventCallback callback = it->callback;
				events.erase(it);
				callback();
				it = events.begin();
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
		VI::AddInitialEvents();
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
		s64 cpu_cycle_overrun = 0, rsp_cycle_overrun = 0;
		while (true) {
			s64 cpu_step_dur = cpu_cycles_per_update - cpu_cycle_overrun;
			s64 rsp_step_dur = cpu_cycles_per_update - rsp_cycle_overrun;
			cpu_cycle_overrun = VR4300::Run(cpu_step_dur);
			rsp_cycle_overrun = RSP::Run(rsp_step_dur);
			CheckEvents(cpu_step_dur);
		}
	}
}