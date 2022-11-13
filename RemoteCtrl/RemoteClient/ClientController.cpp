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
					{WM_SEND_PACKET, &CClientController::OnSendPack},
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
	pClient->Send(CPacket(nCmd,pData,nLength));

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

CClientController::CClientController():m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
{

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
	CClientController::getInstance();
}

CClientController::CHelper::~CHelper()
{
	CClientController::releaseInstance();
}
