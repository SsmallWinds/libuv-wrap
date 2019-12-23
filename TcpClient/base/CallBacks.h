#pragma once;
#include <functional>
#include <memory>

namespace net
{
class TcpClient;
class Buffer;
typedef std::shared_ptr<TcpClient> TcpClientPtr;
typedef std::function<void(const TcpClientPtr &, Buffer *)> MessageCallback;
typedef std::function<void(const TcpClientPtr &)> ConnectionCallback;
typedef std::function<void(const TcpClientPtr &)> CloseCallback;
typedef std::function<void(const TcpClientPtr &)> WriteCompleteCallback;
typedef std::function<void()> TimerCallback;
}; // namespace net
