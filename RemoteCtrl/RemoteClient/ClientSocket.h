#pragma once
#include "pch.h"

#include <list>
#include <string>
#include <vector>
#include <map>
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
using namespace std;

class CPacket
{
public:
	CPacket();                                                          //默认构造
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize, HANDLE hEvent); //封包
	CPacket(const BYTE* pData, size_t& nSize);                          //解包
	CPacket(const CPacket& pack);                                       //拷贝构造
	CPacket& operator=(const CPacket& pack);                            //赋值构造

	int         Size();                          //获取包数据大小
	const char* Data(std::string& strOut) const; //获取包数据

	~CPacket() = default;

	WORD        sHead;   //包头（固定位 FE FF）
	DWORD       nLength; //包长（从命令开始到和校验结束）
	WORD        sCmd;    //命令
	std::string strData; //数据
	WORD        sSum;    //和校验
	//std::string strOut;  //整个包的数据
	HANDLE hEvent;
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


string GetErrorInfo(int wsaErrCode);

class CClientSocket //服务端Socket类 （用于初始化和结束时销毁  单例）
{
public:
	static CClientSocket* getInstance(); //得到一个CClientSocket单例
	bool                  InitSocket();  //配置Socket（绑定、监听）
	int                   DealCommand(); //处理接收到的消息

	bool     GetFilePath(std::string& strPath); //获取文件路径
	bool     GetMouseEvent(MOUSEEVENT& mouse);
	CPacket& GetPacket();
	void     CloseSocket();

	void UpdateAddress(int nIP, int nPort);

	bool SendPacket(const CPacket& pack,std::list<CPacket>& lstPacks, bool isAutoClose = true);


private:
	std::map<HANDLE, bool> m_mapAutoClosed;
	bool m_bAutoClose;
	std::list<CPacket>              m_lstSend;
	std::map<HANDLE, list<CPacket>&> m_mapAck;


	int m_nIP;
	int m_nPort;


	vector<char> m_buffer;
	SOCKET       m_sock;
	CPacket      m_packet;


	static void threadEntry(void* arg);
	void        threadFunc();

	bool Send(const char* pData, int nSize); //发送消息
	bool Send(const CPacket& pack);          //发送数据

	CClientSocket& operator=(const CClientSocket& ss) = delete; //禁用赋值构造 实现单例
	CClientSocket(const CClientSocket& ss);                     //拷贝构造 实现单例

	/* 初始化WSA socket环境 */
	CClientSocket();

	BOOL InitSockEnv();


	~CClientSocket(); //关闭socket 和 WSACleanup

	static void releaseInstance(); //释放CClientSocket内存

	static CClientSocket* m_instance; //实现单例

	class CHelper //辅助构建单例
	{
	public:
		CHelper();

		~CHelper();
	};

	static CHelper m_helper;
};

//extern CClientSocket server;
