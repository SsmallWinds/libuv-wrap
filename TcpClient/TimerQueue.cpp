#include "TimerQueue.h"
#include "EventLoop.h"

using namespace net;
using namespace std;

std::atomic_int64_t TimerQueue::s_timerId;

TimerQueue::TimerQueue(EventLoop* loop) :m_loop(loop)
{

}

int64_t TimerQueue::runAfter(int64_t delay, TimerCallback& cb)
{
	auto* tm = new timer();
	tm->timeId = ++s_timerId;
	tm->timeout = delay;
	tm->repeat = 0;
	tm->cb = std::move(cb);
	tm->queue = this;
	m_loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, tm));
	return tm->timeId;
}

int64_t TimerQueue::runEvery(int64_t repeat, TimerCallback& cb)
{
	auto* tm = new timer();
	tm->timeId = ++s_timerId;
	tm->timeout = 0;
	tm->repeat = repeat;
	tm->cb = std::move(cb);
	tm->queue = this;
	m_loop->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, tm));
	return tm->timeId;
}

void TimerQueue::cancel(int64_t timeId)
{
	m_loop->runInLoop(std::bind(&TimerQueue::stopInloop, this, timeId));
}

void TimerQueue::addTimerInLoop(timer* tm)
{
	uv_timer_init(m_loop->uvLoop(), &tm->timer);
	tm->timer.data = tm;
	uv_timer_start(&tm->timer, timeHandle, tm->timeout, tm->repeat);
}

void TimerQueue::stopInloop(int64_t timeId)
{
	auto tm = m_timers.find(timeId);
	if (tm == m_timers.end())
	{
		return;
	}

	uv_timer_stop(&tm->second->timer);
	uv_close(reinterpret_cast<uv_handle_t*>(&tm->second->timer), timeClose);
}

void TimerQueue::timeHandle(uv_timer_t* handle)
{
	auto* tm = static_cast<timer*>(handle->data);
	tm->cb();
	if (tm->repeat == 0)
	{
		uv_timer_stop(handle);
		uv_close(reinterpret_cast<uv_handle_t*>(handle), timeClose);
	}
}

void TimerQueue::timeClose(uv_handle_t* handle)
{
	auto* tm = static_cast<timer*>(handle->data);
	tm->queue->removeTimer(tm->timeId);
}

void TimerQueue::removeTimer(int64_t timeId)
{
	m_timers.erase(timeId);
}
