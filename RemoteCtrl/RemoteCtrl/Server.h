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
	OVERLAPPED m_overlapped;
	DWORD m_operator;  //操作 
	std::vector<char> m_buffer;
	ThreadWorker m_worker;  //处理函数
	CServer* m_server; //服务器对象
};

template<Koperator>
class AcceptOverlapped;
typedef AcceptOverlapped<KAccept> ACCEPTOVERLAPPED;

class CClient
{
public:
	CClient();
	~CClient();

	void  SetOverlapped(PCLIENT& ptr);

	operator SOCKET();
	operator PVOID();
	operator LPOVERLAPPED();
	operator LPDWORD();

	sockaddr_in* GetLoaclAddr();
	sockaddr_in* GetRemoteAddr();
private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isbusy;
};



template<Koperator>
class AcceptOverlapped: public KOverlapped,ThreadFuncBase
{
public:
	AcceptOverlapped();


	int AcceptWorker();

	PCLIENT m_client;
};


template<Koperator>
class RecvOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped() :m_operator(KRecv), m_worker(this, &RecvOverlapped::RecvWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*265);
	}
	int RecvWorker()
	{
		//TODO:
	}
};
typedef RecvOverlapped<KRecv> RECVOVERLAPPED;

template<Koperator>
class SendOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	SendOverlapped() :m_operator(KSend), m_worker(this, &SendOverlapped::SendWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}
	int SendWorker()
	{
		//TODO:
	}
};
typedef SendOverlapped<KSend> SENDOVERLAPPED;

template<Koperator>
class ErrorOverlapped : public KOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped():m_operator(KError),m_worker(this, &ErrorOverlapped::ErrorWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker()
	{
		//TODO:
	}
};
typedef ErrorOverlapped<KAccept> ERROROVERLAPPED;



class CServer:public ThreadFuncBase
{
public:
	CServer(const std::string& ip="0,0,0,0",short port = 9527);
	~CServer();

	bool StartServic();

	bool NewAccept();

private:
	void CreateSocket();
	
	int threadIocp();


private:
	ThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET,std::shared_ptr<CClient>> m_client;
};

#include "Server.inl"

template<Koperator op>
int AcceptOverlapped<op>::AcceptWorker()
{
	INT lLength = 0, rLength = 0;
	if (*(LPDWORD)*m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16, (sockaddr**)m_client->GetLoaclAddr(), &lLength,
			(sockaddr**)m_client->GetRemoteAddr(), &rLength);

		if (!m_server->NewAccept())
		{
			return -2;
		}
	}
	return -1;
}

template<Koperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_operator = KAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}