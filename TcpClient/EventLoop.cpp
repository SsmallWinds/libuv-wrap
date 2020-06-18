#include "EventLoop.h"
#include <assert.h>

using namespace net;
namespace
{
	thread_local net::EventLoop* t_loopInThisThread = 0;
} // namespace

EventLoop::EventLoop() : m_loop((uv_loop_t*)malloc(sizeof(uv_loop_t))), m_async(), m_timer(this)
{
	uv_loop_init(m_loop);
}

EventLoop::~EventLoop()
{
	if (m_loop != nullptr)
	{
		uv_loop_close(m_loop);
		free(m_loop);
		m_loop = nullptr;
	}
}

int EventLoop::init()
{
	if (t_loopInThisThread == 0)
	{
		t_loopInThisThread = this;
	}
	else
	{
		assert(0);
	}
	m_async.data = this;
	return uv_async_init(m_loop, &m_async, onAsync);
}

void EventLoop::doLoop()
{
	init();
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

void EventLoop::stop()
{
	runInLoop(std::bind(&EventLoop::stopInLoop, this));
}

void EventLoop::onAsync(uv_async_t* handle)
{
	auto* ep = (EventLoop*)handle->data;
	ep->doQueue();
}

void EventLoop::walkCallBack(uv_handle_t* handle, void* arg)
{
	if (!uv_is_closing(handle)) {
		uv_close(handle, nullptr);
	}
}

void EventLoop::stopInLoop()
{
	uv_walk(m_loop, walkCallBack, nullptr);
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