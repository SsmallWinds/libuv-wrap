#pragma once

#include <string>
#include <memory>
#include <uv.h>
#include "EventLoop.h"
#include "base/Buffer.h"
#include "base/CallBacks.h"

namespace net
{

	class TcpClient : public std::enable_shared_from_this<TcpClient>
	{
	public:
		TcpClient(EventLoop* loop, const std::string& name = "client");
		int send(const char* data, int len);
		int connect(const char* ip, int port);

		void setConnectionCallback(ConnectionCallback cb)
		{
			m_connectionCallback = std::move(cb);
		}
		void setMessageCallback(MessageCallback cb)
		{
			m_messageCallback = std::move(cb);
		}
		void setCloseCallback(CloseCallback cb)
		{
			m_closeCallBack = std::move(cb);
		}

		std::string name()const 
		{
			return m_name;
		}

	private:
		static void allocCb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void onClose(uv_handle_t* handle);
		static void onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
		static void onConnect(uv_connect_t* connect, int status);
		static void onWriteDone(uv_write_t* req, int status);
		void sendInLoop(const StringPiece& message);

	private:
		EventLoop* m_loop;
		std::string m_name;
		uv_tcp_t m_tcp;
		uv_connect_t m_connectReq;
		uv_shutdown_t m_shutdownReq;
		uv_write_t m_writeReq;
		Buffer m_buf;
		ConnectionCallback m_connectionCallback;
		MessageCallback m_messageCallback;
		CloseCallback m_closeCallBack;
	};
}; // namespace net
