#pragma once

#include <string>
#include <memory>
#include <uv.h>
#include "EventLoop.h"
#include <Buffer.h>
#include <CallBacks.h>

namespace net
{

	class TcpClient : public std::enable_shared_from_this<TcpClient>
	{
	public:
		TcpClient(EventLoop* loop, const std::string& name = "client");

		void send(const char* data, int len);
		int connect(const char* ip, int port);
		void close();

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

		void setRetry(bool retry)
		{
			m_retry = retry;
		}

	private:
		enum class StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
		static void allocCb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void onClose(uv_handle_t* handle);
		static void onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
		static void onConnect(uv_connect_t* connect, int status);
		static void onWriteDone(uv_write_t* req, int status);
		void sendInLoop(const char* message, size_t size);
		void closeInloop();
		static std::string GetUVError(int errcode)
		{
			std::string err;
			if (0 == errcode) {
				return err;
			}
			auto tmpChar = uv_err_name(errcode);
			if (tmpChar) {
				err = tmpChar;
				err += ":";
			}
			else {
				err = "unknown system errcode " + std::to_string((long long)errcode);
				err += ":";
			}
			tmpChar = uv_strerror(errcode);
			if (tmpChar) {
				err += tmpChar;
			}
			return err;
		}

	private:

		EventLoop* m_loop;
		std::string m_name;
		uv_tcp_t m_tcp;
		uv_connect_t m_connectReq;
		Buffer m_buf;
		ConnectionCallback m_connectionCallback;
		MessageCallback m_messageCallback;
		CloseCallback m_closeCallBack;
		StateE m_state;
		int m_port;
		std::string m_ip;
		bool m_retry;
		std::string m_recBuf;
		int m_retryCount;
		int m_retryInterval;
		bool m_inited;
	};
}; // namespace net