#pragma once
#include <map>
#include <MSWSock.h>

#include "CThread.h"
#include "Queue.hpp"

#pragma warning(disable:4407)


enum Koperator
{
	KNone,
	KAccept,
	KRecv,
	KSend,
	KError
};

class CServer;
class CClient;
typedef std::shared_ptr<CClient> PCLIENT;

class KOverlapped
{
public:
	OVERLAPPED        m_overlapped;
	DWORD             m_operator; //操作 
	std::vector<char> m_buffer;
	ThreadWorker      m_worker; //处理函数
	CServer*          m_server; //服务器对象
	PCLIENT           m_client; //对应的客户端
	WSABUF            m_wsabuffer;
};

template <Koperator>
class AcceptOverlapped;
typedef AcceptOverlapped<KAccept> ACCEPTOVERLAPPED;

template <Koperator>
class RecvOverlapped;
typedef RecvOverlapped<KRecv> RECVOVERLAPPED;

template <Koperator>
class SendOverlapped;
typedef SendOverlapped<KSend> SENDOVERLAPPED;


class CClient
{
public:
	CClient();
	~CClient();

	inline void SetOverlapped(PCLIENT& ptr);

	operator SOCKET();
	operator PVOID();
	operator LPOVERLAPPED();
	operator LPDWORD();
	LPWSABUF RecvWSABuffer();
	LPWSABUF SendWSABuffer();

	DWORD& flags();

	sockaddr_in* GetLoaclAddr();
	sockaddr_in* GetRemoteAddr();
	size_t       GetBufferSize() const;

	int Recv();
private:
	SOCKET                            m_sock;
	DWORD                             m_received;
	DWORD                             m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::shared_ptr<RECVOVERLAPPED>   m_recv;
	std::shared_ptr<SENDOVERLAPPED>   m_send;

	std::vector<char> m_buffer;
	size_t            m_used; //已使用缓冲区大小
	sockaddr_in       m_laddr;
	sockaddr_in       m_raddr;
	bool              m_isbusy;
};


template <Koperator>
class AcceptOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();

	int AcceptWorker();

	PCLIENT m_client;
};


template <Koperator>
class RecvOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped();

	int RecvWorker();
};


template <Koperator>
class SendOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	SendOverlapped();

	int SendWorker();
};

typedef SendOverlapped<KSend> SENDOVERLAPPED;

template <Koperator>
class ErrorOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped();

	int ErrorWorker();
};

typedef ErrorOverlapped<KAccept> ERROROVERLAPPED;


class CServer : public ThreadFuncBase
{
public:
	CServer(const std::string& ip = "0,0,0,0", short port = 9527);
	~CServer();

	bool StartServic();

	bool NewAccept();


private:
	void CreateSocket();

	int threadIocp();


private:
	ThreadPool                                 m_pool;
	HANDLE                                     m_hIOCP;
	SOCKET                                     m_sock;
	sockaddr_in                                m_addr;
	std::map<SOCKET, std::shared_ptr<CClient>> m_client;
};

#include "Server.inl"
