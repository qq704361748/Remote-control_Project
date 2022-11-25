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
#include <MSWSock.h>
#include "Server.hpp"

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

void iocp();

int main()
{
	if (!CTools::Init()) return 1;

	iocp();
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
	}*/
	return 0;
}

class COverlapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped()
	{
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(OVERLAPPED));
		memset(&m_buffer, 0, sizeof(m_buffer));
	}
};



void iocp()
{
	//SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	/*SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		CTools::ShowError();
		return;
	}
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);

	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0); //为什么要再创建一次

	sockaddr_in addr;
	addr.sin_family           = PF_INET;
	addr.sin_addr.S_un.S_addr = inet_addr("0,0,0,0");
	addr.sin_port             = htons(9527);

	bind(sock, (sockaddr*)&addr, sizeof(addr));
	listen(sock, 5);

	COverlapped Overlapped;
	Overlapped.m_operator = 1;


	DWORD received = 0;
	if (AcceptEx(sock, client,Overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received,
		&Overlapped.m_overlapped) == FALSE) {
		CTools::ShowError();
	}
	Overlapped.m_operator = 1;
	Overlapped.m_operator = 1;
	WSASend();



	while (true) {
		//代表一个线程
		LPOVERLAPPED pOverlapped = NULL;
		DWORD        transferred = 0;
		DWORD        key         = 0;
		if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITE)) {
			COverlapped* pO = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped);

			switch (pO->m_operator) {
			case 1:
				

			}
		}
	}*/

	CServer server;
	server.StartServic();
	getchar();

}
