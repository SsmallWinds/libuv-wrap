#include "TcpClient.h"

using namespace net;

TcpClient::TcpClient(EventLoop* loop, const std::string& name) :
	m_loop(loop), m_name(name), m_tcp(), m_connectReq(), m_shutdownReq(), m_writeReq(),
	m_state(StateE::kConnecting)
{
}

void TcpClient::send(const char* data, int len)
{
	if (m_state != StateE::kConnected)
	{
		return;
	}
	StringPiece sp(data, len);
	if (m_loop->isInLoop())
	{
		sendInLoop(sp);
	}
	else
	{
		m_loop->runInLoop(std::bind(&TcpClient::sendInLoop, this, sp.as_string()));
	}
}

int TcpClient::connect(const char* ip, int port)
{
	int ret = 0;
	sockaddr_in addr;
	ret = uv_ip4_addr(ip, port, &addr);
	if (ret != 0)
	{
		return ret;
	}

	ret = uv_tcp_init(m_loop->uvLoop(), &m_tcp);
	if (ret != 0)
	{
		return ret;
	}

	m_tcp.data = this;
	ret = uv_tcp_connect(&m_connectReq, &m_tcp, reinterpret_cast<const struct sockaddr*>(&addr), onConnect);
	return ret;
}

void TcpClient::allocCb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	auto* client = static_cast<TcpClient*>(handle->data);
	*buf = uv_buf_init(client->m_buf.appendSpace(suggested_size), static_cast<unsigned int>(suggested_size));
}

void TcpClient::onClose(uv_handle_t* handle)
{
	auto* client = static_cast<TcpClient*>(handle->data);
	client->m_closeCallBack(client->shared_from_this());
	client->m_state = StateE::kDisconnected;
}

void TcpClient::onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
	if (nread > 0)
	{
		auto* client = static_cast<TcpClient*>(tcp->data);
		client->m_messageCallback(client->shared_from_this(), &client->m_buf);
	}

	if (nread < 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(tcp), onClose);
	}
}

void TcpClient::onConnect(uv_connect_t* connect, int status)
{
	auto* client = static_cast<TcpClient*>(connect->handle->data);
	if (status < 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(connect->handle), onClose);
		return;
	}

	int ret = uv_read_start(connect->handle, allocCb, onRead);
	if (ret != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(connect->handle), onClose);
		return;
	}
	client->m_state = StateE::kConnected;
	client->m_connectionCallback(client->shared_from_this());
}

void TcpClient::onWriteDone(uv_write_t* req, int status)
{
	if (status != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(req), onClose);
	}
}

void TcpClient::sendInLoop(const StringPiece& message)
{
	if (m_state != StateE::kConnected)
	{
		return;
	}
	int res = 0;
	uv_buf_t buf;
	buf.base = const_cast<char*>(message.data());
	buf.len = message.size();
	res = uv_write(&m_writeReq, reinterpret_cast<uv_stream_t*>(&m_tcp), &buf, 1, onWriteDone);
	if (res != 0) 
	{
		uv_close(reinterpret_cast<uv_handle_t*>(&m_tcp), onClose);
	}
}
