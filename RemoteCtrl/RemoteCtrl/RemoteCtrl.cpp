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
#include <conio.h>
#include "Tools.h"
#include "Queue.hpp"

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


bool Init()
{
	HMODULE hModule = ::GetModuleHandle(nullptr);

	if (hModule == nullptr) {
		wprintf(L"错误: GetModuleHandle 失败\n");
		return false;
	}
	if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
		// TODO: 在此处为应用程序的行为编写代码。
		wprintf(L"错误: MFC 初始化失败\n");
		return false;
	}
	return true;
}


enum
{
	IocpListEmpty,
	IocpListPush,
	IocpListPop
};


typedef struct IocpParam
{
	int                    nOperator;
	std::string            strData;
	_beginthread_proc_type cbFunc; //回调
	IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL)
	{
		nOperator = op;
		strData   = sData;
		cbFunc    = cb;
	}

	IocpParam()
	{
		nOperator = -1;
	}
} IOCP_PARAM;

void threadmain(void* arg)
{
	std::list<std::string> lstString;
	DWORD                  dwTransferred = 0;
	ULONG_PTR              CompletionKey = 0;
	OVERLAPPED*            pOverlapped   = NULL;
	int                    count         = 0, count0 = 0;
	while (GetQueuedCompletionStatus(arg, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {

		if (dwTransferred == 0 || CompletionKey == 0) {
			printf("thread is prepare to exit!\r\n");
			break;
		}

		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush) {
			lstString.push_back(pParam->strData);
			count0++;
		} else if (pParam->nOperator == IocpListPop) {
			std::string* pStr = NULL;
			if (lstString.size() > 0) {
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(pStr);
			}
			count++;
		} else if (pParam->nOperator == IocpListEmpty) {
			lstString.clear();
		}

		delete pParam;

	}
	printf("thread exit count %d count0 %d\r\n", count, count0);
}


void threadQueueEntry(void* arg)
{
	threadmain(arg);
	_endthread();
}

void func(void* arg)
{
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list:%s\r\n", pstr->c_str());
		delete pstr;
	} else {
		printf("list is empty,no data\r\n");
	}
}


void test()
{
	//printf("press any key to exit ... \r\n");
	CQueue<std::string> lstStrings;

	ULONGLONG tick0 = GetTickCount64(), tick = GetTickCount64(), total = GetTickCount64();

	while (GetTickCount64() - total <= 1000) {
		//if (GetTickCount64() - tick0 > 13) 
		{
			lstStrings.PushBack("hello world");
			tick0 = GetTickCount64();
		}
	}
	printf("exit done! %d\r\n", lstStrings.Size());

	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {
		//if (GetTickCount64() - tick > 20) 
		{
			std::string str;
			lstStrings.PopFront(str);
			tick = GetTickCount64();
			//printf("pop from queue:%s\r\n", str.c_str());
		}
		//Sleep(1);
	}

	printf("exit done! %d\r\n", lstStrings.Size());
	lstStrings.Clear();

	std::list<std::string> lstData;
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {
		lstData.push_back("hello world");

	}
	printf("lstData push done! %d\r\n", lstData.size());
	total = GetTickCount64();
	while (GetTickCount64() - total <= 1000) {
		if (lstData.size() > 0) lstData.pop_front();
	}
	printf("lstData pop done! %d\r\n", lstData.size());
}

int main()
{
	if (!Init()) return 1;

	for (int i = 0; i < 10; i++) {
		test();
	}


	/*if (CTools::IsAdmin()) {
		OutputDebugString(TEXT("Current is run as administrator!\r\n"));
		AfxMessageBox(TEXT("Current is run as administrator!\r\n"));
	}
	else {
		OutputDebugString(TEXT("Current is run as normal user!\r\n"));
		AfxMessageBox(TEXT("Current is run as normal user!\r\n"));
	}

	if (!Init()) return 1;

	//WriteStartupDir(StartUpPath);

	CCommand cmd;

	int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);

	switch (ret) {
	case -1: MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
		exit(0);
		break;

	case -2: MessageBox(NULL, TEXT("多次无法正常接入用户，结束程序"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
		exit(0);
		break;
	}
	return 0;*/
}
