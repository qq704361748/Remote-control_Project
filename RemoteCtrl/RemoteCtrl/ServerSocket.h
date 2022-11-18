#pragma once
#include "pch.h"
#include <list>
#include "framework.h"
#include "Packet.h"


typedef void (*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket&); //业务回调函数


class CServerSocket //服务端Socket类 （用于初始化和结束时销毁  单例）

{
public:
	static CServerSocket* getInstance(); //得到一个CServerSocket单例
	int    Run(SOCKET_CALLBACK callback, void* arg, short port = 9527);
protected:
	bool     InitSocket(short port);             //配置Socket（绑定、监听）
	bool     AcceptClient();                     //接收Client的Socket连接请求
	int      DealCommand();                      //处理接收到的消息
	bool     Send(const char* pData, int nSize); //发送消息
	bool     Send(CPacket& pack);                //发送数据
	void     CloseClient();                      //断开连接
private:
	SOCKET_CALLBACK m_callback;
	void*           m_arg;
	SOCKET          m_sock;   //服务端用于监听的socket
	SOCKET          m_client; //用于服务端收发消息的socket
	CPacket         m_packet;
	CServerSocket&  operator=(const CServerSocket& ss) = delete; //禁用赋值构造 实现单例
	CServerSocket(const CServerSocket& ss);                      //拷贝构造 实现单例

	/* 初始化WSA socket环境 */
	CServerSocket();

	BOOL InitSockEnv();


	~CServerSocket(); //关闭socket 和 WSACleanup

	static void releaseInstance(); //释放CServerSocket内存

	static CServerSocket* m_instance; //实现单例


	class CHelper //辅助构建单例
	{
	public:
		CHelper();

		~CHelper();
	};

	static CHelper m_helper;
};

//extern CServerSocket server;
