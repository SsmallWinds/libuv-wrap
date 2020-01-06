#pragma once
#include <functional>
#include <mutex>
#include <uv.h>
#include <thread>
#include <vector>
#include "TimerQueue.h"

namespace net
{
	class EventLoop
	{

		typedef std::function<void()> Functor;

	public:
		EventLoop(uv_loop_t* loop);
		int init();

		void doLoop();
		void runInLoop(Functor cb);
		uv_loop_t* uvLoop();
		bool isInLoop();
		static EventLoop* currentLoop();
		void assertInLoopThread();


		int64_t runAfter(int64_t delay, TimerCallback& cb) 
		{
			return m_timer.runAfter(delay, cb);
		}
		int64_t runEvery(int64_t repeat, TimerCallback& cb) 
		{
			return m_timer.runEvery(repeat, cb);
		}
		void cancel(int64_t timeId) 
		{
			m_timer.cancel(timeId);
		}

	private:
		static void onAsync(uv_async_t* handle);
		void queueInLoop(Functor cb);
		void doQueue();

	private:
		std::thread::id m_tid;
		uv_loop_t* m_loop;
		uv_async_t m_async;
		std::mutex m_mutex;
		std::vector<Functor> m_funcs;
		TimerQueue m_timer;
	};
} // namespace net
