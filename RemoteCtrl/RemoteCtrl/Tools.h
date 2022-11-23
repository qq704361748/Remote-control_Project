#pragma once
#define StartUpPath (TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))
//(TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))
//C:\Users\Administrator\OneDrive\桌面
//(TEXT("C:\\Users\\Administrator\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"))


class CTools
{
public:
	static void Dump(BYTE* pData, size_t nSize); //输出磁盘信息
	static bool IsAdmin();
	static void WriteStartupDir(const CString& strPath);
	static bool Init();//初始化MFC
	static void ShowError();
};
