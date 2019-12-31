#include "EventLoop.h"
#include <assert.h>

using namespace net;
namespace
{
	thread_local net::EventLoop* t_loopInThisThread = 0;
} // namespace

EventLoop::EventLoop(uv_loop_t* loop) : m_loop(loop), m_async(),m_timer(this)
{
}

int EventLoop::init()
{
	int ret = 0;
	m_async.data = this;
	return uv_async_init(m_loop, &m_async, onAsync);
}

void EventLoop::doLoop()
{
	if (t_loopInThisThread == 0)
	{
		t_loopInThisThread = this;
	}
	else
	{
		assert(0);
	}
	m_tid = std::this_thread::get_id();
	uv_run(m_loop, UV_RUN_DEFAULT);
}

void EventLoop::runInLoop(Functor cb)
{
	if (isInLoop())
	{
		cb();
	}
	else
	{
		queueInLoop(std::move(cb));
	}
}

uv_loop_t* EventLoop::uvLoop()
{
	return m_loop;
}

EventLoop* EventLoop::currentLoop()
{
	return t_loopInThisThread;
}

void EventLoop::onAsync(uv_async_t* handle)
{
	auto* ep = (EventLoop*)handle->data;
	ep->doQueue();
}

bool EventLoop::isInLoop()
{
	return m_tid == std::this_thread::get_id();
}

void EventLoop::queueInLoop(Functor cb)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_funcs.emplace_back(std::move(cb));
	}

	uv_async_send(&m_async);
}

void EventLoop::doQueue()
{
	std::vector<Functor> functors;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		functors.swap(m_funcs);
	}

	for (const Functor& functor : functors)
	{
		functor();
	}
}