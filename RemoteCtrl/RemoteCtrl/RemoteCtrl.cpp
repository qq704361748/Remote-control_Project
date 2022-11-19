// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <list>
#include <atlimage.h>
#include "Command.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define StartUpPath (TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))
//(TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))
//C:\Users\Administrator\OneDrive\桌面
//(TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))

//#pragma warning(disable:4966)

// #pragma comment ( linker, "/subsystem:windows /entry:WinMainCRTStartup" )
// #pragma comment ( linker, "/subsystem:windows /ENTRY:mainCRTStartup" )
// #pragma comment ( linker,"/subsystem:console /entry:WinMainCRTStartup" )
// #pragma comment ( linker,"/subsystem:console /entry:mainCRTStartup" )

// 唯一的应用程序对象

CWinApp theApp;

void WriteStartupDir(const CString& strPath);

int main()
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(nullptr);


	if (hModule != nullptr) {
		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			nRetCode = 1;
		} else {

			CCommand cmd;
			WriteStartupDir(StartUpPath);
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

			switch (ret) {
			case -1: MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
				break;

			case -2: MessageBox(NULL, TEXT("多次无法正常接入用户，结束程序"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
				exit(0);
				break;
			}
		}

	} else {
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}
	return nRetCode;
}


void WriteStartupDir(const CString& strPath)//写文件到开机目录
{//通过修改开机启动文件夹来实现开机启动
	TCHAR sPath[MAX_PATH] = _T("");
	GetModuleFileName(NULL, sPath, MAX_PATH);
	int ret = CopyFile(sPath, strPath, FALSE);

	CString strCmd;
	CString file;
	file = strPath;
	strCmd.Format(_T("copy \"%s\" \"%s\""), sPath,file.GetBuffer());

	USES_CONVERSION;
	system(W2A(strCmd.GetBuffer()));
	if (ret == 1) {
		MessageBox(NULL, TEXT("设置开机启动成功！"), TEXT("提示"), NULL);
	}
	//fopen CFile system(copy) CopyFile OpenFile


}