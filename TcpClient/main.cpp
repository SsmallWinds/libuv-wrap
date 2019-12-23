#include <iostream>
#include "TcpClient.h"
#include "TimerQueue.h"

using namespace std;
using namespace net;

EventLoop* loop;
TcpClientPtr client;
TimerQueue* tq;

void on_timer()
{
	client->send("on_time\r\n", 9);
}

void on_close(const TcpClientPtr& client)
{
	cout << "on_close:" << client->name() << endl;
}

void on_msg(const TcpClientPtr& client, Buffer* buf)
{
	auto msg = buf->retrieveAllAsString();
	cout << msg << endl;
	client->send(msg.c_str(), static_cast<int>(msg.size()));
}

void on_connected(const TcpClientPtr& client)
{
	cout << "on_connected:" << client->name() << endl;
	client->send("hello", 6);

	TimerCallback fun = std::bind(on_timer);
	tq->runEvery(2000, fun);
}

int main()
{
	cout << "hello" << endl;
	loop = new EventLoop(uv_default_loop());
	client = make_shared<TcpClient>(loop);
	tq = new TimerQueue(loop);

	client->setMessageCallback(on_msg);
	client->setConnectionCallback(on_connected);
	client->setCloseCallback(on_close);

	client->connect("127.0.0.2", 9999);
	loop->doLoop();
}