// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent) {}

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
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint cur = point;
		CRect  clientRect;
		if (isScreen)ScreenToClient(&point); //全局客户区域

		m_picture.GetWindowRect(clientRect);
		float x = m_nObjWidth / clientRect.Width();
		return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
	}
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetTimer(0, 45, NULL);

	return TRUE; // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->m_isExist) {

			SetStretchBltMode(m_picture.GetDC()->GetSafeHdc(), HALFTONE);

			//this->MoveWindow(NULL,NULL, pParent->m_image.GetWidth() / 2, pParent->m_image.GetHeight() / 2);
			if (m_nObjWidth == -1)m_nObjWidth = pParent->m_image.GetWidth();
			if (m_nObjHeight == -1)m_nObjHeight = pParent->m_image.GetHeight();


			SetWindowPos(NULL, 0, 0, pParent->m_image.GetWidth() / 2, pParent->m_image.GetHeight() / 2, SWP_NOMOVE);

			//SetWindowPos(NULL, 0, 0, pParent->m_image.GetWidth() , pParent->m_image.GetHeight(), SWP_NOMOVE);
			CRect rect;

			this->GetWindowRect(rect);
			m_picture.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE);

			//pParent->m_image.BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);

			pParent->m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->m_image.Destroy();
			pParent->SetImageStatus();
		} else { }
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 0; //左键
		even.nAction              = 1; //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
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

		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}


	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 0; //左键
		even.nAction              = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 1; //右键
		even.nAction              = 1; //双击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}


	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 0; //右键
		even.nAction              = 2; //按下//TODO:服务端做对应修改
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}


	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint     remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 0; //右键
		even.nAction              = 3; //弹起
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}


	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) { }

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1) {
		CPoint point;
		GetCursorPos(&point);
		CPoint     remote = UserPoint2RemoteScreenPoint(point, true);
		MOUSEEVENT even;
		even.ptXY                 = remote;
		even.nButton              = 0; //左键
		even.nAction              = 0; //单击
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&even);
	}
}
