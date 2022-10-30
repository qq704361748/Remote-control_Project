#pragma once
#include "pch.h"
#include "framework.h"

class CPacket
{
public:
	CPacket();
	CPacket(const BYTE* pData, size_t& nSize);
	CPacket(const CPacket& pack);
	CPacket& operator=(const CPacket& pack);


	~CPacket() = default;

	WORD        sHead;   //包头（固定位 FE FF）
	DWORD       nLength; //包长（从命令开始到和校验结束）
	WORD        sCmd;    //命令
	std::string strData; //数据
	WORD        sSum;    //和校验
};




class CServerSocket //服务端Socket类 （用于初始化和结束时销毁  单例）
{
public:
	static CServerSocket* getInstance(); //静态函数没有this指针，无法直接访问成员变量

	bool InitSocket(); //初始化Socket

	bool AcceptClient(); //接收Client的Socket连接请求

	int DealCommand(); //处理接收到的消息

	bool Send(const char* pData, int nSize); //发送消息

private:
	SOCKET         m_client;
	SOCKET         m_sock;
	CPacket		   m_packet;
	CServerSocket& operator=(const CServerSocket& ss) = delete;

	CServerSocket(const CServerSocket& ss);

	//初始化WSA socket环境
	CServerSocket();
	BOOL InitSockEnv();

	~CServerSocket(); //关闭socket WSACleanup

	static void releaseInstance();

	static CServerSocket* m_instance;

	class CHelper
	{
	public:
		CHelper();

		~CHelper();
	};

	static CHelper m_helper;
};

//extern CServerSocket server;

