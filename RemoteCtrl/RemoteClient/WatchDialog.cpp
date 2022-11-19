// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "ClientController.h"
//#include "RemoteClientDlg.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjHeight = -1;
	m_nObjWidth = -1;

}

CWatchDialog::~CWatchDialog() {}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint cur = point;
		CRect  clientRect;
		if (isScreen) ScreenToClient(&point); //全局客户区域

		m_picture.GetWindowRect(clientRect);  //获取图片大小
		float x = (float)m_nObjWidth / (float)clientRect.Width();
		return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
	}
	return 0;
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	//SetStretchBltMode(m_picture.GetDC()->GetSafeHdc(), HALFTONE);
	m_isFull = false;
	//SetTimer(0, 45, NULL);
	
	return TRUE; // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	/*// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		
		CClientController* pCtrl = CClientController::getInstance();
		if (m_isFull) {
			
			SetStretchBltMode(m_picture.GetDC()->GetSafeHdc(), HALFTONE); 

			if (m_nObjWidth == -1)m_nObjWidth = m_image.GetWidth();
			if (m_nObjHeight == -1)m_nObjHeight = m_image.GetHeight();

			int x = m_image.GetWidth() * 4 /5;
			int y = m_image.GetHeight() * 4 / 5;

			SetWindowPos(NULL, 0, 0, x+16, y+38, SWP_NOMOVE);  // +标题栏像素

			m_picture.SetWindowPos(NULL, 0, 0,x,y, SWP_NOMOVE);


			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, x, y, SRCCOPY);

			m_picture.InvalidateRect(NULL);
			m_image.Destroy();
			m_isFull = false;
			TRACE("更新图片完成！%d %d %08X\r\n", m_nObjWidth, m_nObjHeight,(HBITMAP)m_image);
		} else { }
	}
	CDialog::OnTimer(nIDEvent);*/
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 0; //左键
		even.nAction = 1; //双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&even, sizeof(even));
	}


	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 0; //左键
		even.nAction = 2; //按下

		TRACE(" 1 ---- x=%d  y=%d \r\n", point.x, point.y);

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
	}


	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 0; //左键
		even.nAction = 3; //弹起

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 1; //右键
		even.nAction = 1; //双击

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
	}


	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 1; //右键
		even.nAction = 2; //按下//TODO:服务端做对应修改

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
	}


	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 1; //右键
		even.nAction = 3; //弹起

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
	}


	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY = remote;
		even.nButton = 8;//没有按键
		even.nAction = 0;//移动
		//CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&even, sizeof(even));
		Sleep(300);
	}

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint point;
		GetCursorPos(&point);
		CPoint     remote = UserPoint2RemoteScreenPoint(point, true);
		MOUSEEVENT even;
		even.ptXY    = remote;
		even.nButton = 0; //左键
		even.nAction = 0; //单击

		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&even, sizeof(even));
	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类
	//this->ShowWindow(HIDE_WINDOW);

	//CDialog::OnOK();
}


void CWatchDialog::OnCancel()
{
	// TODO: 在此添加专用代码和/或调用基类

	auto* pCtrl = CClientController::getInstance();

	pCtrl->m_isClosed = true;
	this->ShowWindow(HIDE_WINDOW);
	//CDialog::OnCancel();
}

LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if (lParam == -1 || (lParam == -2)) {
		//TODO:错误处理
	}
	else if (lParam == 1) {
		//对方关闭了套接字
	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL)
		{
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;
			switch (head.sCmd)
			{
			case 6:
			{
				if (!m_isFull) {
					CTools::Bytes2Image(m_image, head.strData);
					SetStretchBltMode(m_picture.GetDC()->GetSafeHdc(), HALFTONE);

					int x = m_image.GetWidth() * 4 / 5;
					int y = m_image.GetHeight() * 4 / 5;

					if (m_nObjWidth == -1)m_nObjWidth = m_image.GetWidth();
					if (m_nObjHeight == -1)m_nObjHeight = m_image.GetHeight();

					
					CRect mpRect;
					m_picture.GetWindowRect(mpRect);
					if(mpRect.Width()!=x || mpRect.Height()!=y) {
						SetWindowPos(NULL, 0, 0, x + 16, y + 38, SWP_NOMOVE);  // +标题栏像素
						m_picture.SetWindowPos(NULL, 0, 0, x, y, SWP_NOMOVE);
					}
					

					m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, x, y, SRCCOPY);

					m_picture.InvalidateRect(NULL);
					m_image.Destroy();
					m_isFull = false;
					TRACE("更新图片完成！%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
				}
				break;
			}
			default:
				break;
			}

		}

	}

	return 0;
}
