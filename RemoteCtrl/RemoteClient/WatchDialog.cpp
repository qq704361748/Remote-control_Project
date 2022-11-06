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
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetTimer(0, 45, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
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
			SetWindowPos(NULL, 0, 0, pParent->m_image.GetWidth() / 2, pParent->m_image.GetHeight() / 2, SWP_NOMOVE);
			//SetWindowPos(NULL, 0, 0, pParent->m_image.GetWidth() , pParent->m_image.GetHeight(), SWP_NOMOVE);
			CRect rect;

			this->GetWindowRect(rect);
			m_picture.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE);

			//pParent->m_image.BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
			
			pParent->m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY );
			m_picture.InvalidateRect(NULL);
			pParent->m_image.Destroy();
			pParent->SetImageStatus();
		} else {
			
		}
	}
	CDialog::OnTimer(nIDEvent);
}

