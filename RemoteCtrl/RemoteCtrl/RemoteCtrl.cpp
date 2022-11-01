﻿// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <list>

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

using namespace std;

void Dump(BYTE* pData, size_t nSize)
{
	string strOut;
	for (size_t i = 0; i < nSize; i++) {
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0)) {
			strOut += "\n";
		}
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() //从1开始的值，1->A,2->B,3->C
{
	string result;
	for (int i = 1; i <= 26; ++i) {
		if (_chdrive(i) == 0) //_chdrive()如果该盘存在，则值为0
		{
			if (result.size() > 0) {
				result += ',';
			}
			result += 'A' + i - 1;
		}
	}

	CPacket pack(1, (BYTE*)result.c_str(), result.size());
	Dump((BYTE*)pack.Data(), pack.Size());
	//CServerSocket::getInstance()->Send(pack);
	return 0;
}


typedef struct file_info
{
	file_info()
	{
		IsInvalid   = FALSE;
		IsDirectory = -1;
		HasNext     = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid;   //是否有效
	BOOL IsDirectory; //是否为目录 0否，1是
	BOOL HasNext;     //是否还有后续 0没有，1有
	char szFileName[256];
}        FILEINFO, *PFILEINFO;


int MakeDirectoryInfo() //获取指定文件夹下的信息
{
	string strPath;
	//list<FILEINFO> listFileInfos;

	if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
		OutputDebugString(TEXT("当前的命令，不是获取文件列表，命令解析错误！"));
		return -1;
	}

	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		finfo.IsInvalid   = TRUE;
		finfo.IsDirectory = TRUE;
		finfo.HasNext     = FALSE;
		memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
		//listFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		OutputDebugString(TEXT("该目录无法访问！"));
		return -2;
	}

	_finddata_t fdata;
	int         hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(TEXT("当前目录没有文件！"));
		return -3;
	}

	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		//listFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
	} while (!_findnext(hfind, &fdata));

	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int RunFile()
{
	string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}


int DownloadFile()
{
	string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	long long data  = 0;
	FILE*     pFile = fopen(strPath.c_str(), "rb");
	if (pFile == NULL) {
		CPacket pack(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(pack);
		return -1;
	}

	if (pFile != NULL) {


		fseek(pFile, 0, SEEK_END);

		data = _ftelli64(pFile);
		CPacket head(4, (BYTE*)&data, 8);
		fseek(pFile, 0, SEEK_SET);

		char   buffer[1024] = "";
		size_t rlen         = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			CPacket pack(4, (BYTE*)&buffer, rlen);
			CServerSocket::getInstance()->Send(pack);
		} while (rlen >= 1024);

		fclose(pFile);
	}
	CPacket pack(4, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

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

			/*CServerSocket* pserver =  CServerSocket::getInstance();
			int count = 0;
			if (pserver->InitSocket() == false)
			{
			    MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
			    exit(0);
			}
			 while (CServerSocket::getInstance() != NULL)
			 {
				 
			     if(pserver->AcceptClient()==false)
			     {
				     if (count >=3)
				     {
			             MessageBox(NULL, TEXT("多次无法正常接入用户，结束程序"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
			             exit(0);
				     }
			         MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
			         count++;
			     }
 
			     int ret = pserver->DealCommand();
			     //TODO:
			    
			 }
			  */
			int nCmd = 1;
			switch (nCmd) {
			case 1: MakeDriverInfo(); //查看磁盘分区
				break;
			case 2: MakeDirectoryInfo(); //查看指定目录下的文件
				break;
			case 3: RunFile(); //打开文件
				break;
			case 4: DownloadFile(); //下载文件
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
