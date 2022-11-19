#include "pch.h"
#include "ClientController.h"


std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;

CClientController* CClientController::m_instance = nullptr;

CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == nullptr) {
		m_instance = new CClientController();
		struct
		{
			UINT    nMsg;
			MSGFUNC func;
		}           MsgFuncs[] = {
					//{WM_SEND_PACK, &CClientController::OnSendPack},
					//{WM_SEND_DATA, &CClientController::OnSendData},
					{WM_SHOW_STATUS, &CClientController::OnShowStatus},
					{WM_SHOW_WATCH, &CClientController::OnShowWatch},
					{static_cast<UINT>(- 1), nullptr}
				};

		for (int i = 0; MsgFuncs[i].func != NULL; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(nullptr, 0,
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

// bool CClientController::SendPacket(const CPacket& pack)
// {
// 	CClientSocket* pClient = CClientSocket::getInstance();
// 	if (pClient->InitSocket() == false) return false;
// 	return pClient->Send(pack);
//
// }

bool CClientController::SendCommandPacket(HWND   hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,
                                          WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();

	return pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
}

int CClientController::GetImage(CImage& image)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	return CTools::Bytes2Image(image, pClient->GetPacket().strData);
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, L"*", strPath, OFN_OVERWRITEPROMPT,
	                L"", &m_remoteDlg, 0, true);

	if (dlg.DoModal() == IDOK) {

		m_strRemote = strPath;
		m_strLocal  = dlg.GetPathName();

		USES_CONVERSION;
		FILE* pFile = fopen(W2A(m_strLocal), "wb+");
		if (pFile == nullptr) {
			AfxMessageBox(TEXT("本地无权限,文件无法创建"));
			m_statusDlg.ShowWindow(SW_HIDE);
			m_remoteDlg.EndWaitCursor();
			return -1;
		}

		string c_m_strRemote(W2A(m_strRemote));

		SendCommandPacket(m_remoteDlg.GetSafeHwnd(), 4, false, (BYTE*)c_m_strRemote.c_str(), c_m_strRemote.size(),
		                  (WPARAM)pFile);
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);

		/*if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}*/

		m_remoteDlg.BeginWaitCursor();

		//m_statusDlg.m_info.SetWindowTextW(L"命令执行中");
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();

		m_statusDlg.m_ProgressBar.SetRange(0, 100);
		m_statusDlg.m_ProgressBar.SetPos(0);
	}
	return 0;
}

void CClientController::StartWathScreen()
{
	m_isClosed = false;

	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	int ret        = m_watchDlg.ShowWindow(SW_SHOWNORMAL);
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::releaseInstance()
{
	if (m_instance != nullptr) {
		delete m_instance;
		m_instance = nullptr;
	}
}


/*
LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	
	return pClient->Send(*pPacket);
}
*/

/*
LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	char* pBuffer = (char*)wParam;
	return pClient->Send(pBuffer,(int)lParam);
}*/

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.ShowWindow(SW_SHOWNORMAL);
}


void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.m_isFull == false) {
			if (GetTickCount64() - nTick < 400) {
				Sleep(400 - static_cast<DWORD>(GetTickCount64() - nTick));

			}
			nTick   = GetTickCount64();
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, nullptr, 0);
			//TODO:添加响应消息函数 wm_send_pack_ack
			//TODO: 控制发送频率
			if (ret == 1) {
				//TRACE("成功发送图片！ ret = %d\r\n", ret);

			} else {
				TRACE("获取图片失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
}

void CClientController::threadWatchScreen(void* arg)
{
	auto thiz = static_cast<CClientController*>(arg);
	thiz->threadWatchScreen();
	_endthread();
}

CClientController::CClientController(): m_watchDlg(&m_remoteDlg), m_statusDlg(&m_remoteDlg)
{
	m_isClosed     = true;
	m_hThreadWatch = INVALID_HANDLE_VALUE;
	m_hThread      = INVALID_HANDLE_VALUE;
	m_nThreadID    = -1;
}

CClientController::~CClientController()
{
	WaitForSingleObject(m_hThread, 100);
}

void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE) {
			auto pmsg   = (MSGINFO*)msg.wParam;
			auto hEvent = (HANDLE)msg.lParam;

			static auto it = m_mapFunc.find(msg.message);

			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
			} else {
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		} else {
			auto it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}

	}
}


unsigned CClientController::threadEntry(void* arg)
{
	auto* thiz = static_cast<CClientController*>(arg);
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
	releaseInstance();
}
