#include "pch.h"
#include "ServerSocket.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();


MouseEvent::MouseEvent()
{
	nAction = 0;
	nButton = -1;
	ptXY.x  = 0;
	ptXY.y  = 0;
}

CServerSocket* CServerSocket::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CServerSocket();
	}
	return m_instance;
}

bool CServerSocket::InitSocket(short port)
{
	if (m_sock == -1) {
		return false;
	}
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family           = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = INADDR_ANY;
	serv_adr.sin_port             = htons(port);

	//绑定
	if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		return false;
	}

	//监听
	if (listen(m_sock, 1) == -1) {
		return false;
	}

	return true;
}

int CServerSocket::Run(SOCKET_CALLBACK callback, void* arg,short port)
{
	// TODO: socket，bind,listen,accept,read,write,close
	bool ret = InitSocket(port);
	if (ret == false) return -1;

	std::list<CPacket> lstPacket;

	m_callback = callback;
	m_arg = arg;

	int count = 0;
	while (true) {
		if (AcceptClient() == false) {
			if (count >= 3) {
				return -2;
			}
			
			count++;
		}

		int ret = DealCommand();
		if (ret > 0) {
			m_callback(m_arg, ret,lstPacket,m_packet);
			if (lstPacket.size() > 0) {
				Send(lstPacket.front());
				lstPacket.pop_front();
			}
		}
		CloseClient();

	}

	return 0;
}


bool CServerSocket::AcceptClient()
{
	TRACE("enter AcceptClient \r\t");
	SOCKADDR_IN client_adr;

	int cli_sz = sizeof(client_adr);
	m_client   = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
	TRACE("m_client = %d\r\n", m_client);
	if (m_client == -1) return false;
	return true;
}

#define BUFFER_SIZE 4096

int CServerSocket::DealCommand()
{
	if (m_client == -1) return -1;
	char* buffer = new char[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;

	while (true) {
		size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
		if (len <= 0) {
			delete[] buffer;
			return -1;
		}
		//TEST
		TRACE("recv %d\r\n", len);

		index += len;
		len      = index;
		m_packet = CPacket((BYTE*)buffer, len);
		if (len > 0) {
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			delete[] buffer;
			return m_packet.sCmd;
		}
	}
	delete[] buffer;
	return -1;
}

bool CServerSocket::Send(const char* pData, int nSize)
{
	if (m_client == -1) return false;
	return send(m_client, pData, nSize, 0) > 0;
}

bool CServerSocket::Send(CPacket& pack)
{
	if (m_client == -1) return false;
	return send(m_client, pack.Data(), pack.Size(), 0) > 0;
}

bool CServerSocket::GetFilePath(std::string& strPath)
{
	if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4 || m_packet.sCmd == 9)  //当命令为，即为获取文件目录
	{
		strPath = m_packet.strData;
		return true;
	}
	return false;
}


bool CServerSocket::GetMouseEvent(MOUSEEVENT& mouse)
{
	if (m_packet.sCmd == 5) {
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEVENT));
		return true;
	}
	return false;
}

CPacket& CServerSocket::GetPacket()
{
	return m_packet;
}

void CServerSocket::CloseClient()
{
	if (m_client != INVALID_SOCKET) {
		closesocket(m_client);
		m_client = INVALID_SOCKET;
	}
	
}


CServerSocket::CServerSocket(const CServerSocket& ss)
{
	m_sock   = ss.m_sock;
	m_client = ss.m_client;
}

CServerSocket::CServerSocket()
{
	m_sock   = INVALID_SOCKET;
	m_client = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {

		MessageBox(NULL, TEXT("无法初始化套接字环境,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
}

CServerSocket::~CServerSocket()
{
	closesocket(m_sock);
	WSACleanup();
}

BOOL CServerSocket::InitSockEnv()
{
	//初始化SOCKET
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}
	return TRUE;
}

void CServerSocket::releaseInstance()
{
	if (m_instance != NULL) {
		CServerSocket* tmp = m_instance;
		m_instance         = NULL;
		delete tmp;
	}
}

CServerSocket::CHelper::CHelper()
{
	CServerSocket::getInstance();
}

CServerSocket::CHelper::~CHelper()
{
	CServerSocket::releaseInstance();
}

