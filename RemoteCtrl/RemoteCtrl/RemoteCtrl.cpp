﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//#pragma warning(disable:4966)

// #pragma comment ( linker, "/subsystem:windows /entry:WinMainCRTStartup" )
// #pragma comment ( linker, "/subsystem:windows /ENTRY:mainCRTStartup" )
// #pragma comment ( linker,"/subsystem:console /entry:WinMainCRTStartup" )
// #pragma comment ( linker,"/subsystem:console /entry:mainCRTStartup" )

// 唯一的应用程序对象

CWinApp theApp;

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
			// TODO: socket，bind,listen,accept,read,write,close

			CCommand cmd;
			CServerSocket* pserver = CServerSocket::getInstance();

			int ret = pserver->Run(&CCommand::RunCommand, &cmd);

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
