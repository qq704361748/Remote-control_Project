// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框


CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}


void CRemoteClientDlg::SetImageStatus(bool isExist)
{
	m_isExist = isExist;
}


void CRemoteClientDlg::threadEntryForWatch(void* arg)
{
	CRemoteClientDlg* m_this = (CRemoteClientDlg*)arg;
	m_this->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	CClientSocket* pClient = NULL;
	do {
		pClient = CClientSocket::getInstance();
		
	} while (pClient == NULL);

	ULONGLONG tick = GetTickCount64();

	while (!m_isClosed) //等价于  while(true)
	{

		if (m_isExist == false) {
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret > 0) {
				std::string strData = pClient->GetPacket().strData;
				BYTE*       pData   = (BYTE*)strData.c_str();
				//TODO:存入CImage
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL) {
					TRACE("内存不足!");
					Sleep(10);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT  hRet    = CreateStreamOnHGlobal(hMem, true, &pStream);
				if (hRet == S_OK) {
					ULONG length = 0;
					pStream->Write(pData, strData.size(), &length);
					LARGE_INTEGER bg = {0};
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);
					if ((HBITMAP)m_image != NULL) {
						m_image.Destroy();
					}
					m_image.Load(pStream);

					m_isExist = true;
				}
			} else {
				Sleep(10);

			}

		} else {
			Sleep(10);
		}


	}
}


void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* m_this = (CRemoteClientDlg*)arg;
	m_this->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int     nListSelected = m_List.GetSelectionMark();
	CString cStrPath      = m_List.GetItemText(nListSelected, 0);

	CFileDialog dlg(FALSE, (LPCTSTR)TEXT("*"), m_List.GetItemText(nListSelected, 0), OFN_OVERWRITEPROMPT,
	                (LPCTSTR)TEXT(""), this, 0, true);


	if (dlg.DoModal() == IDOK) {
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		cStrPath            = GetPath(hSelected) + cStrPath;
		TRACE("%s\r\n", cStrPath);
		USES_CONVERSION;
		std::string strPath(W2A(cStrPath));

		FILE* pFile = fopen(W2A(dlg.GetPathName()), "wb+");
		if (pFile == NULL) {
			AfxMessageBox(TEXT("本地无权限,文件无法创建"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}


		int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)strPath.c_str());
		if (ret < 0) {
			AfxMessageBox(TEXT("执行下载命令失败!!!"));
			TRACE("执行下载命令失败  %d\r\n", ret);
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		CClientSocket* pClient = CClientSocket::getInstance();
		long long      nLength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nLength == 0) {
			AfxMessageBox(TEXT("文件长度为0,或无法读取"));
			pClient->CloseSocket();
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		long long nCount = 0;

		while (nCount < nLength) {
			ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox(TEXT("传输失败!!!"));
				TRACE("传输失败  %d\r\n", ret);
				pClient->CloseSocket();
				m_dlgStatus.ShowWindow(SW_HIDE);
				EndWaitCursor();
				return;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
			nCount += pClient->GetPacket().strData.size();
		}
		fclose(pFile);
		pClient->CloseSocket();
	}

	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	AfxMessageBox(TEXT("传输完成"));

	return;
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTreeSelected = m_Tree.GetSelectedItem();
	CString   strPath       = GetPath(hTreeSelected);
	m_List.DeleteAllItems();
	USES_CONVERSION;
	std::string    str(W2A(strPath));
	int            nCmd    = SendCommandPacket(2, false, (BYTE*)str.c_str(), str.length());
	PFILEINFO      pInfo   = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pInfo->HasNext) {
		TRACE("[%s] sidir  %d \r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CStringW(pInfo->szFileName) == L"." || CStringW(pInfo->szFileName) == L"..") {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) { break; }
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(CStringW(pInfo->szFileName), hTreeSelected, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		} else {
			m_List.InsertItem(0, CStringW(pInfo->szFileName));
		}

		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) {
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}


	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);

	HTREEITEM hTreeSelectded = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelectded == NULL) {
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelectded) == NULL) return;

	DeleteTreeChildrenItem(hTreeSelectded);
	m_List.DeleteAllItems();

	CString strPath = GetPath(hTreeSelectded);
	USES_CONVERSION;
	string         str(W2A(strPath));
	int            ncmd    = SendCommandPacket(2, false, (BYTE*)str.c_str(), str.length());
	PFILEINFO      pInfo   = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	CClientSocket* pClient = CClientSocket::getInstance();
	while (pInfo->HasNext) {
		TRACE("Name [%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CStringW(pInfo->szFileName) == L"." || CStringW(pInfo->szFileName) == L"..") {
				int cmd = pClient->DealCommand();
				TRACE("ACK: %d\r\n", cmd);
				if (cmd < 0) {
					break;
				}
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(CStringW(pInfo->szFileName), hTreeSelectded, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		} else {
			m_List.InsertItem(0, CStringW(pInfo->szFileName));
		}

		int cmd = pClient->DealCommand();
		TRACE("ACK: %d\r\n", cmd);
		if (cmd < 0) {
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}


	pClient->CloseSocket();
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool           ret     = pClient->InitSocket(m_server_address, _ttoi(m_nPort));
	if (!ret) {
		AfxMessageBox(TEXT("网络初始化失败！"));
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	pClient->Send(pack);
	TRACE("Send ret %d\r\n ", ret);
	int cmd = pClient->DealCommand();
	TRACE("ACK:%d\r\n", cmd);
	if (bAutoClose) {
		pClient->CloseSocket();
	}
	return cmd;
}


BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_BN_CLICKED(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_BN_CLICKED(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_BN_CLICKED(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CRemoteClientDlg::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CRemoteClientDlg::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL    bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);  // 设置大图标
	SetIcon(m_hIcon, FALSE); // 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort          = TEXT("9527");
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isExist = false;

	dlg.Create(IDD_DLG_WATCH);


	return TRUE; // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int   cxIcon = GetSystemMetrics(SM_CXICON);
		int   cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	SendCommandPacket(1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(TEXT("命令处理失败！"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	m_Tree.DeleteAllItems();
	string drivers = pClient->GetPacket().strData;
	//wstring dr;
	string dr; //本机默认编码为UTF-8，visual studio默认编码为UTF-16，需用wstring
	drivers += ",";

	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ':';
			HTREEITEM hTemp = m_Tree.InsertItem((LPCTSTR)CStringW(dr.c_str()), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strtmp;
	do {
		strtmp = m_Tree.GetItemText(hTree);
		strRet = strtmp + L"\\" + strRet;
		hTree  = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	//TRACE("[%s]\r\n", strRet);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL) {
			m_Tree.DeleteItem(hSub);
		}
	} while (hSub != NULL);
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) {
		return;
	}
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);

	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	//添加线程函数
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	Sleep(50);
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowTextW(TEXT("命令执行中"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();


	//TODO:大文件传输用额外的线程处理
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString   strPath   = GetPath(hSelected);
	int       nSelected = m_List.GetSelectionMark();
	CString   strFile   = m_List.GetItemText(nSelected, 0);
	strFile             = strPath + strFile;

	USES_CONVERSION;
	std::string cstrPath(W2A(strFile));
	int         ret = SendCommandPacket(9, true, (BYTE*)cstrPath.c_str(), cstrPath.size());
	if (ret < 0) {
		AfxMessageBox(TEXT("删除文件命令执行失败！"));
	}
	LoadFileCurrent();
}

void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString   strPath   = GetPath(hSelected);
	int       nSelected = m_List.GetSelectionMark();
	CString   strFile   = m_List.GetItemText(nSelected, 0);
	strFile             = strPath + strFile;

	USES_CONVERSION;
	std::string cstrPath(W2A(strFile));
	int         ret = SendCommandPacket(3, true, (BYTE*)cstrPath.c_str(), cstrPath.size());
	if (ret < 0) {
		AfxMessageBox(TEXT("打开文件命令执行失败！"));
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wparam, LPARAM lparam)
{
	int cmd = wparam >> 1;
	int ret = 0;

	switch (cmd) {
	case 4:
		ret = SendCommandPacket(wparam >> 1, wparam & 1, (BYTE*)std::string((LPCSTR)lparam).c_str(),
		                        std::string((LPCSTR)lparam).size());
		break;
	case 5: //鼠标操作
		ret = SendCommandPacket(cmd, wparam & 1, (BYTE*)lparam, sizeof(MOUSEEVENT));
		break;
	case 6:
	// ret = SendCommandPacket(cmd, wparam & 1);
	// break;
	case 7:ret = SendCommandPacket(wparam >> 1, wparam & 1);
		break;
	case 8:
		ret = SendCommandPacket(wparam >> 1, wparam & 1);
		break;
	default:
		ret = -1;
	}
	return ret;
}




void CRemoteClientDlg::OnBnClickedBtnStartWatch()  //TODO:未能实现结束非模态对话框进程
{
	static int flag = 0;
	if (flag == 0) {
		flag = 1;
		m_isClosed = false;
	
		//m_isClosed     = false;
		int    ret = dlg.ShowWindow(SW_SHOWNORMAL);
		HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatch, 0, this);
		WaitForSingleObject(hThread, 500);
	} else {
		dlg.ShowWindow(SW_SHOWNORMAL);
	}
	/*m_isClosed = false;
	CWatchDialog dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatch, 0, this);
	dlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(hThread, 500);*/
	

	// dlg.ShowWindow(SW_SHOWNORMAL);
	// GetDlgItem(IDC_BTN_START_WATCH)->EnableWindow(FALSE);
	//dlg.DoModal();
}


void CRemoteClientDlg::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码

	SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
	
}


void CRemoteClientDlg::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}

