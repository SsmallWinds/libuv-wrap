#pragma once
#include <CallBacks.h>
#include <map>
#include <atomic>
#include <memory>
#include <uv.h>

namespace net {

	class TimerQueue;
	class EventLoop;
	struct timer
	{
		int64_t timeId;
		uint64_t timeout;
		uint64_t repeat;
		uv_timer_t uv_timer;
		TimerCallback cb;
		TimerQueue* queue;
	};

	class TimerQueue
	{
	public:
		TimerQueue(EventLoop* loop);

		int64_t runAfter(int64_t delay, TimerCallback& cb);
		int64_t runEvery(int64_t repeat, TimerCallback& cb);
		void cancel(int64_t timeId);

	private:
		void addTimerInLoop(timer* tm);
		void stopInloop(int64_t timeId);
		static void timeHandle(uv_timer_t* handle);
		static void timeClose(uv_handle_t* handle);
		void removeTimer(int64_t timeId);

	private:
		std::map<int64_t, std::unique_ptr<timer>> m_timers;
		static std::atomic<int64_t> s_timerId;
		EventLoop* m_loop;
	};

}; // namespace net
