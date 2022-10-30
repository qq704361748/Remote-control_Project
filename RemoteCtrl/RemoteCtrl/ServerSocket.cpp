#include "pch.h"
#include "ServerSocket.h"


//CServerSocket server;

CServerSocket* CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();


CServerSocket* CServerSocket::getInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new CServerSocket();
	}
	return m_instance;
}

bool CServerSocket::InitSocket()
{
	if (m_sock == -1)
	{
		return false;
	}
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family           = AF_INET;
	serv_adr.sin_addr.S_un.S_addr = INADDR_ANY;
	serv_adr.sin_port             = htons(9527);

	//绑定
	if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
	{
		return false;
	}

	//监听
	if (listen(m_sock, 1) == -1)
	{
		return false;
	}
	return true;
}

bool CServerSocket::AcceptClient()
{
	SOCKADDR_IN client_adr;
	int         cli_sz = sizeof(client_adr);
	m_client           = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
	if (m_client == -1) return false;
	return true;
}

#define BUFFER_SIZE 4096
int CServerSocket::DealCommand()
{
	if (m_client == -1) return -1;
	//char buffer[1024] = { 0 };
	char* buffer = new char[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true)
	{
		size_t len = recv(m_client, buffer+index, BUFFER_SIZE -index, 0);
		if (len <= 0) return -1;

		index += len;
		len = index;
		m_packet=CPacket((BYTE*)buffer, len);
		if (len > 0)
		{
			memmove(buffer, buffer + len, BUFFER_SIZE -len);
			index -= len;
			return m_packet.sCmd;
		}
	}
	return -1;
}

bool CServerSocket::Send(const char* pData, int nSize)
{
	return send(m_client, pData, nSize, 0) > 0;
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
	if (InitSockEnv() == FALSE)
	{
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
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
	{
		return FALSE;
	}
	return TRUE;
}

void CServerSocket::releaseInstance()
{
	if (m_instance != NULL)
	{
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

CPacket::CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}

CPacket::CPacket(const BYTE* pData, size_t& nSize)
{
	size_t i = 0;
	for (; i < nSize ; ++i)
	{ 
		if (*(WORD*)(pData+i) == 0xFEFF)
		{
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}

		if (i+8 > nSize)  //包数据可能不全，或包头未能全部接收
		{
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength+i > nSize)  //包未完全接收到
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;

		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}

		sSum = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0;j<strData.size();j++)
		{
			sum += BYTE(strData[i]) & 0xFF;
		}

		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;
	}
}


CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}

CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack) 
	{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
	}
	return *this;
}

