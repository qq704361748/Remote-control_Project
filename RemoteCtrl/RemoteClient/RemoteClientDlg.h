﻿// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include <string>

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
	// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr); // 标准构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支持

private:
	void LoadFileCurrent();
	void LoadFileInfo();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	/**
	 * \brief 
	 * \param nCmd  \n
	 * 1.查看磁盘分区 \n
	 * 2.查看指定目录下的文件 \n
	 * 3.打开文件 \n
	 * 4.下载文件 \n
	 * 5.鼠标操作 \n
	 * 6.发送屏幕内容 \n
	 * 7.锁机 \n
	 * 8.解锁 \n
	 * 9.删除文件 \n
	 * \return 命令cmd
	 */
	int SendCommandPacket(int nCmd, bool bAutoClose=true,BYTE* pData=NULL,size_t nLength=0);
	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL    OnInitDialog();
	afx_msg void    OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void    OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD        m_server_address;
	CString      m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	
	void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
};
