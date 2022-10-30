#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance() //静态函数没有this指针，无法直接访问成员变量
	{
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	bool InitSocket()
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
		if(listen(m_sock, 1)== -1)
		{
			return false;
		}
		return true;
	}

	bool AcceptClient()
	{
		SOCKADDR_IN client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		return true;
		

	}

	int DealCommand()
	{
		if (m_client == -1) return false;
		char buffer[1024] = { 0 };
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) return -1;

			//TODO:处理命令
		}
	}


	bool Send(const char* pData,int nSize)
	{
		return send(m_client, pData, nSize, 0) > 0;
	}
	
private:
	SOCKET m_client;
	SOCKET m_sock;
	CServerSocket& operator=(const CServerSocket& ss)
	{
	}

	CServerSocket(const CServerSocket& ss)
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}

	CServerSocket()
	{
		m_sock = INVALID_SOCKET;
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, TEXT("无法初始化套接字环境,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}

	~CServerSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	}


	BOOL InitSockEnv()
	{
		//初始化SOCKET
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}

	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance         = NULL;
			delete tmp;
		}
	}

	static CServerSocket* m_instance;

	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}

		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

extern CServerSocket server;
