export module Scheduler;

import Util;

import <vector>;

namespace Scheduler
{
	export
	{
		using EventCallback = void(*)(); /* TODO: make it statically known what callbacks correspond with which event? */
		using Time = s64; /* signed so that we can subtract a duration and check if the result is negative */

		enum class EventType {
			AudioSample,
			CountCompareMatch,
			PiDmaFinish,
			Render,
			SiDmaFinish,
			SpDmaFinish,
			VINewHalfline
		};

		void AddEvent(EventType event, Time cycles_until_fire, EventCallback callback);
		void ChangeEventTime(EventType event, Time cpu_cycles_until_fire);
		void Initialize();
		void RemoveEvent(EventType event);
		void Run();
	}

	struct Event
	{
		EventType event_type;
		Time time_until_fire; /* signed so that we can subtract a duration and check if the result is negative */
		EventCallback callback;
	};

	void CheckEvents();
	void OnRenderEvent();

	constexpr Time cpu_cycles_per_update = 60;
	constexpr Time rsp_cycles_per_update = 2 * cpu_cycles_per_update / 3;
	static_assert(2 * cpu_cycles_per_update == 3 * rsp_cycles_per_update);

	std::vector<Event> events;
}