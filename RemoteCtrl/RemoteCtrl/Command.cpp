#include "pch.h"
#include "Command.h"

#include <atlimage.h>
#include <direct.h>

#include "resource.h"
#include "ServerSocket.h"
#include "Tools.h"
#include <atlimage.h>
#include <corecrt_io.h>


CCommand::CCommand(): threadid(0)
{
	struct
	{
		int     nCmd;
		CMDFUNC func;
	}           data[] = {
				{1, &CCommand::MakeDriverInfo},    //查看磁盘分区
				{2, &CCommand::MakeDirectoryInfo}, //查看指定目录下的文件
				{3, &CCommand::RunFile},           //打开文件
				{4, &CCommand::DownloadFile},      //下载文件
				{5, &CCommand::MouseEvent},        //鼠标操作
				{6, &CCommand::SendScreen},        //发送屏幕内容 ==>发送屏幕截图
				{7, &CCommand::LockMachine},       //锁机
				{8, &CCommand::UnlockMachine},     //解锁
				{9, &CCommand::DeleteLocalFile},   //删除文件
				{1981, &CCommand::TestConnect},    //连接测试
				{-1,NULL},
			};

	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

CCommand::~CCommand() {}


int CCommand::ExcuteCommoand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}

	return (this->*it->second)(lstPacket, inPacket);
}

void CCommand::RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	CCommand* thiz = (CCommand*)arg;

	if (status > 0) {
		int ret = thiz->ExcuteCommoand(status, lstPacket, inPacket);
		if (ret != 0) {
			TRACE("执行命令失败：%d ret = %d\r\n", status, ret);
		}
	} else {
		MessageBox(NULL, TEXT("无法正常接入用户，自动重试"), TEXT("接入用户失败"), MB_OK | MB_ICONERROR);
	}
}

int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) //从1开始的值，1->A,2->B,3->C
{
	std::string result;
	for (int i = 1; i <= 26; ++i) {
		if (_chdrive(i) == 0) //_chdrive()如果该盘存在，则值为0
		{
			if (result.size() > 0) {
				result += ",";
			}
			result +='A' + i - 1;
		}
	}
	lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
	return 0;
}

int CCommand::MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) //获取指定文件夹下的信息
{
	std::string strPath = inPacket.strData;


	if (_chdir(strPath.c_str()) != 0) {
		FILEINFO finfo;

		//listFileInfos.push_back(finfo);
		finfo.HasNext = FALSE;

		memcpy(finfo.szFileName, strPath.c_str(), strlen(strPath.c_str()));

		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		OutputDebugString(TEXT("该目录无法访问！"));
		return -2;
	}

	_finddata_t fdata;
	int         hfind = 0;
	if ((hfind = _findfirst("*", &fdata)) == -1) {
		OutputDebugString(TEXT("当前目录没有文件！"));
		FILEINFO finfo;
		finfo.HasNext = FALSE;

		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return -3;
	}
	int count = 0;
	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("%s\r\n", finfo.szFileName);
		//listFileInfos.push_back(finfo);
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		count++;
	} while (!_findnext(hfind, &fdata));
	TRACE("发送了 %d 次目录\r\n", count);
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
	return 0;
}

int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string strPath = inPacket.strData;
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	lstPacket.push_back(CPacket(3, NULL, 0));
	return 0;
}

int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	USES_CONVERSION;
	std::string strPath =(inPacket.strData);
	long long   data    = 0;
	FILE*       pFile;


	fopen_s(&pFile, strPath.c_str(), "rb");
	if (pFile == NULL) {
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
		
		return -1;
	}

	if (pFile != NULL) {

		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);


		fseek(pFile, 0, SEEK_SET);
		lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));

		char   buffer[1024] = {0};
		size_t rlen         = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
		} while (rlen >= 1024);

		fclose(pFile);
	}
	lstPacket.push_back(CPacket(4, NULL, 0));
	return 0;
}

int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	MOUSEEVENT mouse;
	memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEVENT));

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

	lstPacket.push_back(CPacket(5, NULL, 0));

	return 0;
}

int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
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
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE  pData = (PBYTE)GlobalLock(hMme);
		SIZE_T nSize = GlobalSize(hMme);

		lstPacket.push_back(CPacket(6, pData, nSize));

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


int CCommand::LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
		//_beginthread(threadLockDlg, 0, NULL);
		_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
	}
	lstPacket.push_back(CPacket(7, NULL, 0));
	return 0;
}

int CCommand::UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	//dlg.SendMessage(WM_KEYDOWN, 0x41, 0x01E0001);
	//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x41, 0x01E0001);
	PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0);
	lstPacket.push_back(CPacket(8, NULL, 0));
	return 0;
}


int CCommand::DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string strPath = inPacket.strData;

	USES_CONVERSION;
	DeleteFile(A2W(strPath.c_str()));
	lstPacket.push_back(CPacket(9, NULL, 0));


	return 0;
}


int CCommand::TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	lstPacket.push_back(CPacket(1981, NULL, 0));

	return 0;
}

unsigned CCommand::threadLockDlg(void* arg)
{
	CCommand* thiz = (CCommand*)arg;
	thiz->threadLockDlgMain();
	_endthreadex(0);
	return 0;
}

void CCommand::threadLockDlgMain()
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
			//TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x1B) {
				
				break;
			}
		}
	}

	::ShowWindow(::FindWindow(TEXT("Shell_TrayWnd"), NULL), SW_SHOW); //显示任务栏
	ShowCursor(true);
	ClipCursor(NULL);
	dlg.DestroyWindow(); //显示鼠标
}
