#pragma once
#include <functional>
#include <mutex>
#include <uv.h>
#include <thread>
#include <vector>

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
		void assertInLoopThread()
		{
			if (!isInLoop())
			{
				//TODO::
			}
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
	};
} // namespace net
