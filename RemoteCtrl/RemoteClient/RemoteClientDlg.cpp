// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"

#include <list>

#include "afxdialogex.h"
#include "WatchDialog.h"
#include "ClientController.h"

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


void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTreeSelected = m_Tree.GetSelectedItem();
	CString   strPath       = GetPath(hTreeSelected);
	m_List.DeleteAllItems();
	USES_CONVERSION;
	std::string str(W2A(strPath));
	int         nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)str.c_str(),
	                                                                       str.length());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();


	while (pInfo->HasNext) {
		TRACE("[%s] sidir  %d \r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == L"." || CString(pInfo->szFileName) == L"..") {
				int cmd = CClientController::getInstance()->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) { break; }
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(CString(pInfo->szFileName), hTreeSelected, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		} else {
			m_List.InsertItem(0, CString(pInfo->szFileName));
		}

		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) {
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}


	//CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);

	//HTREEITEM hTreeSelectded = m_Tree.GetSelectedItem();
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) {
		return;
	}
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) return;

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();

	CString strPath = GetPath(hTreeSelected);

	USES_CONVERSION;
	string str(W2A(strPath));

	std::list<CPacket> lstPacks;
	int ncmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)str.c_str(),
	                                                               str.length(),(WPARAM)hTreeSelected);
	//int            ncmd = CClientController::getInstance()->SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(),&lstPacks);
	if (lstPacks.size() > 0) {
		std::list<CPacket>::iterator it = lstPacks.begin();
		for (; it != lstPacks.end(); it++) {
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
			if (pInfo->HasNext == false)continue;

			if (pInfo->IsDirectory) {
				if (CString(pInfo->szFileName) == TEXT(".") || CString(pInfo->szFileName) == TEXT("..")) {
					continue;
				}
				HTREEITEM hTemp = m_Tree.InsertItem(CString(pInfo->szFileName), hTreeSelected, TVI_LAST);
				m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			} else {
				m_List.InsertItem(0, CString(pInfo->szFileName));
			}
		}
	}
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

	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CRemoteClientDlg::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CRemoteClientDlg::OnBnClickedBtnUnlock)
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
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
	m_server_address = 0xC0A85F8B; // 0x7F000001
	m_nPort          = TEXT("9527");
	UpdateData(FALSE);

	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));


	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);


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
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	std::list<CPacket> lstPackets;
	int                ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true,NULL, 0);
	if (ret == 0) {
		AfxMessageBox(L"命令处理失败");
	}
	/*CPacket& head = lstPackets.front();


	m_Tree.DeleteAllItems();
	string drivers = head.strData;
	string dr;

	drivers += ",";

	for (size_t i = 0; i < drivers.size(); i++) {
		if (drivers[i] == ',') {
			dr += ':';
			HTREEITEM hTemp = m_Tree.InsertItem((LPCTSTR)CString(dr.c_str()), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}*/
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strtmp;
	do {
		strtmp = m_Tree.GetItemText(hTree);
		strRet = strtmp + TEXT("\\") + strRet;
		hTree  = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	TRACE("[%s]\r\n", strRet);
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
	int     nListSelected = m_List.GetSelectionMark();
	CString cStrPath      = m_List.GetItemText(nListSelected, 0);

	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	cStrPath            = GetPath(hSelected) + cStrPath;

	//添加线程函数
	int ret = CClientController::getInstance()->DownFile(cStrPath);
	if (ret != 0) {
		MessageBox(L"下载失败");
		TRACE("下载失败 ret = %d\r\n", ret);
	}
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)cstrPath.c_str(),
	                                                              cstrPath.size());
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
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)cstrPath.c_str(),
	                                                              cstrPath.size());
	if (ret < 0) {
		AfxMessageBox(TEXT("打开文件命令执行失败！"));
	}
}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWathScreen();
}


void CRemoteClientDlg::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}


void CRemoteClientDlg::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, _ttoi(m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		//TODO:错误处理
	} else if (lParam == 1) {
		//对方关闭了套接字
	} else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;

			switch (head.sCmd) {
			case 1:
				{
					m_Tree.DeleteAllItems();
					string drivers = head.strData;
					string dr;

					drivers += ",";

					for (size_t i = 0; i < drivers.size(); i++) {
						if (drivers[i] == ',') {
							dr += ':';
							HTREEITEM hTemp = m_Tree.InsertItem((LPCTSTR)CString(dr.c_str()), TVI_ROOT, TVI_LAST);
							m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
							dr.clear();
							continue;
						}
						dr += drivers[i];
					}
				}
				break;
			case 2:
				{
					PFILEINFO pInfo = (PFILEINFO)head.strData.c_str();
					if (pInfo->HasNext == false)break;

					if (pInfo->IsDirectory) {
						if (CString(pInfo->szFileName) == TEXT(".") || CString(pInfo->szFileName) == TEXT("..")) {
							break;
						}
						HTREEITEM hTemp = m_Tree.InsertItem(CString(pInfo->szFileName), (HTREEITEM)lParam, TVI_LAST);
						m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
					} else {
						m_List.InsertItem(0, CString(pInfo->szFileName));
					}
				}
				break;
			case 3:
				TRACE("Run file done! \r\n");
				break;
			case 4:
				{
				static LONGLONG length = 0,index = 0;
				if (length == 0) {
					length = *(long long*)head.strData.c_str();
					if (length == 0)
					{
						AfxMessageBox(L"文件长度为0,或无法读取");
						CClientController::getInstance()->DownLoadEnd();
					} 
					
				}else if (length > 0 && index >=length) {
					fclose((FILE*)lParam);
					length = 0;
					index = 0;
					CClientController::getInstance()->DownLoadEnd();
				}
				else {
					FILE* pFile = (FILE*)lParam;
					fwrite(head.strData.c_str(), 1,head.strData.size(), pFile);
					index += head.strData.size();
				}
			}
				break;
			case 9:
				TRACE("Delete file done! \r\n");
				break;
			case 1981:
				TRACE("test connection success! \r\n");
				break;
			default:
				TRACE("Unknow data received! %d\r\n", head.sCmd);
				break;
			}
		}

	}

	return 0;
}
