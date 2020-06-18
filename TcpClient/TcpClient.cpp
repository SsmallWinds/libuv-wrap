#include "TcpClient.h"
#include <spdlog/spdlog.h>
#include <assert.h>

using namespace net;

TcpClient::TcpClient(EventLoop* loop, const std::string& name) :
	m_loop(loop), m_name(name), m_tcp(), m_connectReq(),
	m_state(StateE::kConnecting), m_port(0), m_retry(false),
	m_retryCount(0), m_retryInterval(1000), m_inited(false)
{
	m_recBuf.resize(65536);
}

void TcpClient::send(const char* data, int len)
{
	//copy memory to buf
	auto* buf = static_cast<char*>(malloc(len));
	if (buf == nullptr)
	{
		return;
	}
	memcpy(buf, data, len);

	SPDLOG_DEBUG("tcp client send:msg size = {},client status ={}", len, m_state);

	if (m_loop->isInLoop())
	{
		sendInLoop(buf, len);
	}
	else
	{
		m_loop->runInLoop(std::bind(&TcpClient::sendInLoop, this, buf, len));
	}
}


int TcpClient::connect(const char* ip, int port)
{
	int ret = 0;
	m_state = StateE::kConnecting;
	m_ip = ip;
	m_port = port;
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
	SPDLOG_DEBUG("client connect:ip = {},port={}", ip, port);
	return ret;
}

void net::TcpClient::close()
{
	SPDLOG_DEBUG("close, client status = {}", m_state);
	if (m_state != StateE::kConnected)
	{
		return;
	}

	m_loop->runInLoop(std::bind(&TcpClient::closeInloop, this));
}

void TcpClient::allocCb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	auto* client = static_cast<TcpClient*>(handle->data);
	assert(suggested_size <= client->m_recBuf.size());
	*buf = uv_buf_init(const_cast<char*>(client->m_recBuf.c_str()), client->m_recBuf.size());
}

void TcpClient::onClose(uv_handle_t* handle)
{
	SPDLOG_DEBUG("client OnClose");
	auto* client = static_cast<TcpClient*>(handle->data);
	client->m_state = StateE::kDisconnected;
	client->m_closeCallBack(client->shared_from_this());

	if (client->m_inited && client->m_retry)
	{
		SPDLOG_DEBUG("client start reconnect,retry count={}", client->m_retryCount);
		client->m_retryCount++;
		if (client->m_retryCount > 60)
		{
			client->m_retryInterval *= 2;
			if (client->m_retryInterval >= 60000)
			{
				client->m_retryInterval = 60000;
			}
		}
		client->m_loop->runAfter(client->m_retryInterval, std::bind(&TcpClient::connect, client, client->m_ip.c_str(), client->m_port));
	}
}

void TcpClient::onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
	if (nread > 0)
	{
		auto* client = static_cast<TcpClient*>(tcp->data);
		client->m_buf.append(buf->base, nread);
		client->m_messageCallback(client->shared_from_this(), &client->m_buf);
	}

	if (nread < 0)
	{
		SPDLOG_DEBUG("{}", GetUVError(nread));
		uv_close(reinterpret_cast<uv_handle_t*>(tcp), onClose);
	}
}

void TcpClient::onConnect(uv_connect_t* connect, int status)
{
	auto* client = static_cast<TcpClient*>(connect->handle->data);
	if (status < 0)
	{
		SPDLOG_DEBUG("{}", GetUVError(status));
		uv_close(reinterpret_cast<uv_handle_t*>(connect->handle), onClose);
		return;
	}

	int ret = uv_read_start(connect->handle, allocCb, onRead);
	if (ret != 0)
	{
		SPDLOG_DEBUG("{}", GetUVError(ret));
		uv_close(reinterpret_cast<uv_handle_t*>(connect->handle), onClose);
		return;
	}
	client->m_inited = true;
	client->m_state = StateE::kConnected;
	client->m_retryCount = 0;
	client->m_connectionCallback(client->shared_from_this());
}

void TcpClient::onWriteDone(uv_write_t* req, int status)
{
	if (status != 0)
	{
		uv_close(reinterpret_cast<uv_handle_t*>(req->handle), onClose);
		SPDLOG_DEBUG("onWriteDone error: errorCode = {}, errorMsg = {}", status, GetUVError(status));
	}
	//free buf
	free(req->data);
	//free req
	free(req);
}

void TcpClient::sendInLoop(const char* message, size_t size)
{
	m_loop->assertInLoopThread();
	if (m_state != StateE::kConnected)
	{
		SPDLOG_DEBUG("message unsended, client state is {},message size = {}", m_state, size);
		return;
	}
	int res = 0;
	uv_buf_t buf;
	buf.base = const_cast<char*>(message);
	buf.len = size;

	auto* req = static_cast<uv_write_t*>(malloc(sizeof(uv_write_t)));
	if (req == nullptr)
	{
		//free buf
		free(buf.base);
		return;
	}

	req->data = buf.base;
	//buf may not have been sent complete, then set to queue
	res = uv_write(req, reinterpret_cast<uv_stream_t*>(&m_tcp), &buf, 1, onWriteDone);
	if (res != 0)
	{
		SPDLOG_DEBUG("uv_write error: errorCode = {}, errorMsg = {}, msgSize = {}", res, GetUVError(res), size);
		uv_close(reinterpret_cast<uv_handle_t*>(&m_tcp), onClose);
		//free buf
		free(buf.base);
		//free req
		free(req);
	}
}

void net::TcpClient::closeInloop()
{
	m_inited = false;
	if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_tcp))) 
	{
		SPDLOG_DEBUG("uv_close!");
		uv_close(reinterpret_cast<uv_handle_t*>(&m_tcp), onClose);
	}
	else
	{
		SPDLOG_DEBUG("uv_is_closing!");
	}
}
