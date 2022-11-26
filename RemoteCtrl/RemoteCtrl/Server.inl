#include "Server.hpp"


inline KOverlapped::~KOverlapped()
{
	m_buffer.clear();


}

CClient::CClient()
	: m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this,(SENDCALLBACK)& CClient::SendData)

{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

CClient::~CClient()
{
	m_buffer.clear();
	closesocket(m_sock);
	m_recv.reset();
	m_send.reset();
	m_overlapped.reset();
	m_vecSend.Clear();
}

void CClient::SetOverlapped(PCLIENT& ptr)
{
	m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}

CClient::operator SOCKET()
{
	return m_sock;
}

CClient::operator PVOID()
{
	return &m_buffer[0];
}

CClient::operator LPOVERLAPPED()
{
	return &m_overlapped->m_overlapped;
}

CClient::operator LPDWORD()
{
	return &m_received;
}

inline LPWSABUF CClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

inline LPWSAOVERLAPPED CClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

inline LPWSABUF CClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

inline LPWSAOVERLAPPED CClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}

inline DWORD& CClient::flags()
{
	return m_flags;
}

sockaddr_in* CClient::GetLoaclAddr()
{
	return &m_laddr;
}

sockaddr_in* CClient::GetRemoteAddr()
{
	return &m_raddr;
}

inline size_t CClient::GetBufferSize() const
{
	return m_buffer.size();
}

inline int CClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data()+ m_used, m_buffer.size()- m_used, 0);
	if(ret <= 0 )return -1;
	m_used += (size_t)ret;
	//TODO:解析数据
	CTools::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}

inline int CClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if(m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}

inline int CClient::SendData(std::vector<char>& data)
{
	if(m_vecSend.Size() >0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received,
			m_flags, &m_send->m_overlapped, NULL);
		if(ret!=0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CTools::ShowError();
			return -1;
		}
	}
	return 0;
}


CServer::CServer(const std::string& ip, short port) : m_pool(10)
{
	
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_sock = INVALID_SOCKET;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	m_addr.sin_port = htons(port);
}

inline CServer::~CServer()
{
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
	for (;it != m_client.end();it++) {
		it->second.reset();
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool CServer::StartServic()
{
	CreateSocket();

	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CServer::threadIocp));
	if (!NewAccept()) return false;

	return true;

}

void CServer::CreateSocket()
{
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
		sizeof(opt));
}

bool CServer::NewAccept()
{
	PCLIENT pClient(new CClient());
	pClient->SetOverlapped(pClient);
	m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));


	if (!AcceptEx(m_sock, *pClient, *pClient, 0,
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		*pClient, *pClient))
	{
		TRACE("%d\r\n", WSAGetLastError());
		if(WSAGetLastError()!= WSA_IO_PENDING) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		
	}
	return true;
}

inline void CServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

int CServer::threadIocp()
{
	DWORD       transferred = 0;
	ULONG_PTR   CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey,
		&lpOverlapped, INFINITE)) {
		if (CompletionKey != 0) {
			auto* pOverlapped = CONTAINING_RECORD(lpOverlapped, KOverlapped,
				m_overlapped);
			TRACE("pOverlapped->m_operator %d \r\n", pOverlapped->m_operator);

			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator) {
			case KAccept:
			{
				auto* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case KRecv:
			{
				auto* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case KSend:
			{
				auto* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case KError:
			{
				auto* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			}
		}
		else {
			return -1;
		}

	}
	return 0;
}


/************************************************************/
/************************************************************/
/************************************************************/

template<Koperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_operator = KAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<Koperator op>
int AcceptOverlapped<op>::AcceptWorker()
{
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		sockaddr* plocal = NULL, * premote = NULL;
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16, (sockaddr**)&plocal, &lLength,
			(sockaddr**)&premote, &rLength);

		memcpy(m_client->GetLoaclAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client);

		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1,
			*m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if(ret == SOCKET_ERROR && (WSAGetLastError()!=WSA_IO_PENDING)) {
			//TODO:报错
			TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());
		}

		if (!m_server->NewAccept())
		{
			return -2;
		}
	}
	return -1;
}

template <Koperator op>
RecvOverlapped<op>::RecvOverlapped()
{
	m_operator = KRecv;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024*265);
}

template <Koperator op>
int RecvOverlapped<op>::RecvWorker() {
	int ret = m_client->Recv();
	return ret;
}

template <Koperator op>
SendOverlapped<op>::SendOverlapped()
{
	m_operator = KSend;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024*256);
}

template <Koperator op>
int SendOverlapped<op>::SendWorker() {
	//TODO:



	return -1;
}

template <Koperator op>
ErrorOverlapped<op>::ErrorOverlapped()
{
	m_operator = KError;
	m_worker = ThreadWorker(this, (FUNCTYPE)&ErrorOverlapped<op>::ErrorWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
}

template <Koperator op>
int ErrorOverlapped<op>::ErrorWorker() {
	//TODO:
	return -1;
}


