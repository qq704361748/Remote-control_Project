#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket();                                           //默认构造
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize); //封包
	CPacket(const BYTE* pData, size_t& nSize);           //解包
	CPacket(const CPacket& pack);                        //拷贝构造
	CPacket& operator=(const CPacket& pack);             //赋值构造

	int         Size(); //获取包数据大小
	const char* Data(); //获取包数据

	~CPacket() = default;

	WORD        sHead;   //包头（固定位 FE FF）
	DWORD       nLength; //包长（从命令开始到和校验结束）
	WORD        sCmd;    //命令
	std::string strData; //数据
	WORD        sSum;    //和校验
	std::string strOut;  //整个包的数据
};

#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent();
	WORD  nAction; //点击、移动、双击
	WORD  nButton; //左键、右键、中键
	POINT ptXY;    //坐标
}         MOUSEEVENT, *PMOUSEEVENT;

typedef struct file_info
{
	file_info()
	{
		IsInvalid   = FALSE;
		IsDirectory = -1;
		HasNext     = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid;   //是否有效
	BOOL IsDirectory; //是否为目录 0否，1是
	BOOL HasNext;     //是否还有后续 0没有，1有
	char szFileName[256];
}        FILEINFO, *PFILEINFO;


typedef void (*SOCKET_CALLBACK)(void* arg,int status); //业务回调函数


class CServerSocket //服务端Socket类 （用于初始化和结束时销毁  单例）
{
public:
	static CServerSocket* getInstance();          //得到一个CServerSocket单例
	bool                  InitSocket(short port); //配置Socket（绑定、监听）
	int                   Run(SOCKET_CALLBACK callback, void* arg, short port = 9527);
	bool                  AcceptClient();                     //接收Client的Socket连接请求
	int                   DealCommand();                      //处理接收到的消息
	bool                  Send(const char* pData, int nSize); //发送消息
	bool                  Send(CPacket& pack);                //发送数据
	bool                  GetFilePath(std::string& strPath);  //获取文件路径
	bool                  GetMouseEvent(MOUSEEVENT& mouse);   //获取鼠标事件
	CPacket&              GetPacket();                        //获取命令参数
	void                  CloseClient();                      //断开连接
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
