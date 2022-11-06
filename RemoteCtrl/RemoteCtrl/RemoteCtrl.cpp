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
#include "LockDialog.h"

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

CLockDialog dlg;
unsigned    threadid = 0;




int ExcuteCommoand(int nCmd);  

void Dump(BYTE* pData, size_t nSize); //输出磁盘信息
int  MakeDriverInfo();                //获得磁盘信息
int  MakeDirectoryInfo();             //获取指定文件夹下的信息
int  RunFile();                       //运行文件
int  DownloadFile();                  //下载文件
int  MouseEvent();                    //鼠标事件
int  SendScreen();                    //发送屏幕截图
int  LockMachine();                   //锁机
int  UnlockMachine();                 //解锁

unsigned __stdcall threadLockDlg(void* arg); //子线程执行锁机




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

			CServerSocket* pserver = CServerSocket::getInstance();
			int            count   = 0;
			if (pserver->InitSocket() == false) {
				MessageBox(NULL, TEXT("网络初始化异常，未能成功初始化，请检查网络"), TEXT("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			while (CServerSocket::getInstance() != NULL) {

				if (pserver->AcceptClient() == false) {
					if (count >= 3) {
						MessageBox(NULL, TEXT("多次无法正常接入用户，结束程序"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
					count++;
				}
				TRACE("AcceptClient return true\r\n");

				int ret = pserver->DealCommand();
				TRACE("DealCommand ret %d\r\n", ret);
				if (ret > 0) {
					ret = ExcuteCommoand(ret);
					if (ret != 0) {
						TRACE("执行命令失败：%d ret = %d\r\n", pserver->GetPacket().sCmd, ret);
					}
					pserver->CloseClient();
					TRACE("Command has done!\r\n");
				}
			}


		}
	} else {
		// TODO: 更改错误代码以符合需要
		wprintf(L"错误: GetModuleHandle 失败\n");
		nRetCode = 1;
	}

	return nRetCode;
}


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
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int MakeDirectoryInfo() //获取指定文件夹下的信息
{
	string strPath;

	
	//list<FILEINFO> listFileInfos;

	if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
		OutputDebugString(TEXT("当前的命令，不是获取文件列表，命令解析错误！"));
		return -1;
	}

	// if (_chdir(strPath.c_str()) != 0)
	// if (_wchdir(TEXT("E:\\")) != 0)
	//strPath += ":\\";
	// strPath.append(":\\");
	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;
		
		//listFileInfos.push_back(finfo);
		finfo.HasNext = FALSE;
		memcpy(finfo.szFileName, strPath.c_str(), strlen(strPath.c_str()));


		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		OutputDebugString(TEXT("该目录无法访问！"));
		return -2;
	}

	_finddata_t fdata;
	int         hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(TEXT("当前目录没有文件！"));
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		return -3;
	}

	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("%s\r\n", finfo.szFileName);
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
	FILE* pFile;
	fopen_s(&pFile,strPath.c_str(), "rb");
	if (pFile == NULL) {
		CPacket pack(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(pack);
		return -1;
	}

	if (pFile != NULL) {


		fseek(pFile, 0, SEEK_END);

		data = _ftelli64(pFile);
		CPacket head(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(head);
		fseek(pFile, 0, SEEK_SET);

		char   buffer[1024] = {0};
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

int MouseEvent()
{
	MOUSEEVENT mouse;
	if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {

		SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		DWORD nFlags = 0;
		switch (mouse.nButton) {
		case 0: //左键
			nFlags = 1;
			break;
		case 1: //右键
			nFlags = 2;
			break;
		case 2: //中键
			nFlags = 4;
			break;
		case 4: //没有按键
			nFlags = 8;
			break;
		}

		if (nFlags != 8) {
			SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
		}
		switch (mouse.nAction) {
		case 0: //单击
			nFlags |= 0x10;
			break;
		case 1: //双击
			nFlags |= 0x20;
			break;
		case 2: //按下
			nFlags |= 0x40;
			break;
		case 3: //放开
			nFlags |= 0x80;
			break;
		default:
			break;
		}


		switch (nFlags) {
		case 0x11: //左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x21: //左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41: //左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: //左键放开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x12: //右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: //右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42: //右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: //右键放开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x14: //中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24: //中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44: //中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84: //中键放开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;


		case 0x08: //鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}

		CPacket pack(4, NULL, 0);
		CServerSocket::getInstance()->Send(pack);

	} else {
		OutputDebugString(TEXT("获取鼠标操作参数失败！"));
		return -1;
	}


	return 0;
}

int SendScreen()
{
	//
	// 获取多屏幕截图
	//
	/*DISPLAY_DEVICEA allscreen = new DISPLAY_DEVICEA;
	CHAR   DeviceName1[32] = { 0 };
	CHAR   DeviceName2[32] = { 0 };
	allscreen = { 0 };
	//llscreen = new DISPLAY_DEVICEA;
	allscreen.cb = sizeof(DISPLAY_DEVICEA);
	EnumDisplayDevicesA(NULL, 0, &allscreen, EDD_GET_DEVICE_INTERFACE_NAME);
	memcpy(DeviceName1, allscreen.DeviceName, 32);
	EnumDisplayDevicesA(NULL, 1, &allscreen, EDD_GET_DEVICE_INTERFACE_NAME);
	memcpy(DeviceName2, allscreen.DeviceName, 32);

	CImage screena;
	HDC hScreena = CreateDCA(DeviceName2, DeviceName2, NULL, NULL);
	int nBitPerPixela = GetDeviceCaps(hScreena, BITSPIXEL);
	int nWidtha = GetDeviceCaps(hScreena, HORZRES);
	int nHeighta = GetDeviceCaps(hScreena, VERTRES);
	screena.Create(nWidtha, nHeighta, nBitPerPixela);
	BitBlt(screena.GetDC(), 0, 0, nWidtha, nHeighta, hScreena, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreena);
	screena.Save(TEXT("abc2022.png"), Gdiplus::ImageFormatPNG);
	screena.ReleaseDC();*/

	CImage screen;
	HDC    hScreen      = ::GetDC(NULL);
	int    nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int    nWidth       = GetDeviceCaps(hScreen, HORZRES);
	int    nHeight      = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreen);
	HGLOBAL hMme = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMme == NULL) {
		return -1;
	}
	IStream* pStream = NULL;
	HRESULT  ret     = CreateStreamOnHGlobal(hMme, TRUE, &pStream);

	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatJPEG);
		LARGE_INTEGER bg = {0};
		pStream->Seek(bg, STREAM_SEEK_SET,NULL);
		PBYTE   pData = (PBYTE)GlobalLock(hMme);
		SIZE_T  nSize = GlobalSize(hMme);
		CPacket pack(6, pData, nSize);
		CServerSocket::getInstance()->Send(pack);
		GlobalUnlock(hMme);
	}

	// screen.Save(TEXT("test2022.jpg"), Gdiplus::ImageFormatJPEG);
	// screen.Save(TEXT("test2022.png"), Gdiplus::ImageFormatPNG);
	/*int avg1 = 0, avg2 = 0;
	int png=0, jpg=0;
	for (int i = 0;i<100;i++) {
		DWORD tick = GetTickCount64();

		screen.Save(TEXT("test2022.png"), Gdiplus::ImageFormatPNG);
		png = GetTickCount64() - tick;
		// TRACE("PNG %d\n", png);
		
		tick = GetTickCount64();
		screen.Save(TEXT("test2022.jpg"), Gdiplus::ImageFormatJPEG);
		jpg = GetTickCount64() - tick;
		// TRACE("JPG %d\n", jpg);
		avg1 += png;
		avg2 += jpg;
	}

	TRACE("PNG %d\n", avg1 / 100);
	TRACE("JPG %d\n", avg2 / 100);*/

	pStream->Release();
	GlobalFree(hMme);
	screen.ReleaseDC();
	return 0;
}


int LockMachine()
{
	if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
		//_beginthread(threadLockDlg, 0, NULL);
		_beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
	}
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

int UnlockMachine()
{
	//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
	//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
	PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
	CPacket pack(8, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
	return 0;
}

unsigned __stdcall threadLockDlg(void* arg)
{
	dlg.Create(IDD_DIALOG_INFO, NULL);
	dlg.ShowWindow(SW_SHOW);

	CRect rect;
	rect.left = 0;
	rect.top  = 0;

	//获取屏幕物理像素宽高比
	HDC hScreen = ::GetDC(NULL);
	int nWidth  = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	ReleaseDC(NULL, hScreen);

	rect.right  = nWidth;
	rect.bottom = nHeight;
	//rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	//rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	dlg.MoveWindow(rect);

	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //窗口置顶

	ShowCursor(false);                                                //隐藏鼠标
	::ShowWindow(::FindWindow(TEXT("Shell_TrayWnd"), NULL), SW_HIDE); //隐藏任务栏
	//限制鼠标活动范围
	dlg.GetWindowRect(rect);
	rect.right  = rect.left + 1;
	rect.bottom = rect.top + 1;
	ClipCursor(rect);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x1B) {
				break;
			}
		}
	}

	::ShowWindow(::FindWindow(TEXT("Shell_TrayWnd"), NULL), SW_SHOW); //显示任务栏
	ShowCursor(true);
	dlg.DestroyWindow(); //显示鼠标
	_endthreadex(0);
	return 0;
}

int TestConnect()
{
	CPacket pack(1981, NULL, 0);

	bool ret = CServerSocket::getInstance()->Send(pack);
	TRACE("Send ret = %d\r\n", ret);
	return 0;
}

int DeleteLocalFile()
{
	string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
	USES_CONVERSION;
	DeleteFile(A2W(strPath.c_str()));
	CPacket pack(9, NULL, 0);
	bool ret = CServerSocket::getInstance()->Send(pack);


	return 0;
}

int ExcuteCommoand(int nCmd)
{
	//int nCmd = 7;
	int ret = 0;
	switch (nCmd) {
	case 1: ret = MakeDriverInfo(); //查看磁盘分区
		break;
	case 2: ret = MakeDirectoryInfo(); //查看指定目录下的文件
		break;
	case 3: ret = RunFile(); //打开文件
		break;
	case 4: ret = DownloadFile(); //下载文件
		break;
	case 5: ret = MouseEvent(); //鼠标操作
		break;
	case 6: ret = SendScreen(); //发送屏幕内容 ==>发送屏幕截图
		break;
	case 7: ret = LockMachine(); //锁机
		break;
	case 8: ret = UnlockMachine(); //解锁
		break;
	case 9:ret = DeleteLocalFile();  //删除文件
		break;
	case 1981: ret = TestConnect();  //连接测试
	}
	return ret;
}
