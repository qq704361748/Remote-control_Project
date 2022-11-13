#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"



#define WM_SEND_PACK (WM_USER+1)  //发送包数据
#define WM_SEND_DATA (WM_USER+2)  //发送数据
#define WM_SHOW_STATUS (WM_USER+3)  //展示状态
#define WM_SHOW_WATCH (WM_USER+4)  //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)  //自定义消息处理

class CClientController
{
public:
	static CClientController* getInstance(); //单例入口

	int InitController(); //初始化

	int Invoke(CWnd*& pMainWnd); //启动

	LRESULT MySendMessage(MSG msg); //发送消息
protected:
	static void releaseInstance();

	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);

private:
	CClientController();
	~CClientController();

	void                      threadFunc();
	static unsigned __stdcall threadEntry(void* arg);

	typedef struct MsgInfo
	{
		MSG     msg;
		LRESULT result;

		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}

		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m, sizeof(MSG));
		}

		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m, sizeof(MSG));

			}
			return *this;
		}


		void Clear()
		{
			result = 0;
			memset(&msg, 0, sizeof(MSG));
		}
	} MSGINFO;


	typedef LRESULT (CClientController::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC>       m_mapFunc;
	
	CWatchDialog                         m_watchDlg;
	CRemoteClientDlg                     m_remoteDlg;
	CStatusDlg                           m_statusDlg;
	HANDLE                               m_hThread;
	unsigned                             m_nThreadID;


	static CClientController* m_instance;

	class CHelper
	{
	public:
		CHelper();
		~CHelper();
	};

	static CHelper m_helper;
};
