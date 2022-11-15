#include "pch.h"
#include "ClientController.h"


std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;

CClientController* CClientController::m_instance =NULL;

CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		struct
		{
			UINT    nMsg;
			MSGFUNC func;
		}           MsgFuncs[] = {
					{WM_SEND_PACK, &CClientController::OnSendPack},
					{WM_SEND_DATA, &CClientController::OnSendData},
					{WM_SHOW_STATUS, &CClientController::OnShowStatus},
					{WM_SHOW_WATCH, &CClientController::OnShowWatch},
					{(UINT)- 1,NULL}
				};

		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
	                                   &CClientController::threadEntry,
	                                   this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	m_watchDlg.Create(IDD_DLG_WATCH, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

LRESULT CClientController::MySendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL)return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&msg, (LPARAM)&info);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);
	return info.result;
}

void CClientController::UpdateAddress(int nIP, int nPort)
{
	CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
}

int CClientController::DealCommand()
{
	return CClientSocket::getInstance()->DealCommand();
}

void CClientController::CloseSocket()
{
	CClientSocket::getInstance()->CloseSocket();
}

bool CClientController::SendPacket(const CPacket& pack)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	if (pClient->InitSocket() == false) return false;
	return pClient->Send(pack);

}

int CClientController::SendCommandPacket(int nCmd , bool bAutoClose, BYTE* pData, size_t nLength)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	if (pClient->InitSocket() == false) return false;

	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	pClient->Send(CPacket(nCmd,pData,nLength,hEvent));

	int cmd = DealCommand();
	TRACE("ACK:%d\r\n", cmd);
	if (bAutoClose) {
		CloseSocket();
	}
	return cmd;

}

int CClientController::GetImage(CImage& image)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	return CTools::Bytes2Image(image, pClient->GetPacket().strData);
}

int CClientController::DownFile(CString strPath)
{
	
	CFileDialog dlg(FALSE, (LPCTSTR)TEXT("*"), strPath, OFN_OVERWRITEPROMPT,
		(LPCTSTR)TEXT(""), &m_remoteDlg, 0, true);

	if (dlg.DoModal() == IDOK) {

		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		
		m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);

		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}

		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowTextW(L"命令执行中");
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();

	} 
	return 0;
}

void CClientController::StartWathScreen()
{
	m_isClosed = false;
	
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	int    ret = m_watchDlg.ShowWindow(SW_SHOWNORMAL);
	WaitForSingleObject(m_hThreadWatch, 500);

}

void CClientController::releaseInstance()
{
	if (m_instance != NULL) {
		delete m_instance;
		m_instance = NULL;
	}
}



LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	
	return pClient->Send(*pPacket);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	char* pBuffer = (char*)wParam;
	return pClient->Send(pBuffer,(int)lParam);
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);

}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.ShowWindow(SW_SHOWNORMAL);
}

void CClientController::threadDownloadFile()
{
	USES_CONVERSION;
	FILE* pFile = fopen(W2A(m_strLocal), "wb+");
	if (pFile == NULL)
	{
		AfxMessageBox(TEXT("本地无权限,文件无法创建"));
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();

	string c_m_strRemote(W2A(m_strRemote));  //宽字符转窄字符

	do
	{
		int ret = SendCommandPacket(4, false, (BYTE*)c_m_strRemote.c_str(), c_m_strRemote.size());

		long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0)
		{
			AfxMessageBox(L"文件长度为0,或无法读取");
			pClient->CloseSocket();
			return;
		}

		long long nCount = 0;
		while (nCount < nLength)
		{
			ret = pClient->DealCommand();
			if (ret < 0)
			{
				AfxMessageBox(TEXT("传输失败!!!"));
				TRACE("传输失败  %d\r\n", ret);
				pClient->CloseSocket();
				return;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);

	fclose(pFile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(TEXT("下载完成"), TEXT("完成"));


}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadWatchScreen()
{
	Sleep(50);

	while (!m_isClosed) //等价于  while(true)
	{
		
		if (m_watchDlg.m_isFull == false) {
			int ret = SendCommandPacket(6);

			if (ret == 6) {
				if (GetImage(m_remoteDlg.m_image) == 0) {
					m_watchDlg.m_isFull = true;
				}
				else {
					TRACE(TEXT("获取图片失败！\r\n"));
				}
			}
			else {
				Sleep(10);
			}
		}
		else {
			Sleep(10);
		}
	}

}

void CClientController::threadWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

CClientController::CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
{
	m_isClosed = true;
	m_hThreadWatch = INVALID_HANDLE_VALUE;
	m_hThreadDownload = INVALID_HANDLE_VALUE;

	m_hThread = INVALID_HANDLE_VALUE;
	m_nThreadID = -1;


}

CClientController::~CClientController()
{
	WaitForSingleObject(m_hThread, 100);
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pmsg   = (MSGINFO*)msg.wParam;
			HANDLE   hEvent = (HANDLE)msg.lParam;

			static std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);

			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			} else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		} else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}

	}
}


unsigned CClientController::threadEntry(void* arg)
{
	auto* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

CClientController::CHelper::CHelper()
{
	//CClientController::getInstance();
}

CClientController::CHelper::~CHelper()
{
	CClientController::releaseInstance();
}
