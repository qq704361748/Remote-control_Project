
CClient::CClient() : m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));
}

CClient::~CClient()
{
	closesocket(m_sock);
}

void CClient::SetOverlapped(PCLIENT& ptr)
{
	m_overlapped->m_client = ptr;
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

sockaddr_in* CClient::GetLoaclAddr()
{
	return &m_laddr;
}

sockaddr_in* CClient::GetRemoteAddr()
{
	return &m_raddr;
}


CServer::CServer(const std::string& ip, short port) : m_pool(10)
{
	m_hIOCP = INVALID_HANDLE_VALUE;
	m_sock = INVALID_SOCKET;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	m_addr.sin_port = htons(port);
}

inline CServer::~CServer() {}

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
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	return true;
}

int CServer::threadIocp()
{
	DWORD       transferred = 0;
	ULONG_PTR   CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey,
		&lpOverlapped, INFINITE)) {
		if (transferred > 0 && CompletionKey != 0) {
			auto* pOverlapped = CONTAINING_RECORD(lpOverlapped, KOverlapped,
				m_overlapped);
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
