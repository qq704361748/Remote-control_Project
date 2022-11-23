#include "pch.h"
#include "Tools.h"
#include <string>


void CTools::Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
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


void CTools::ShowError()
{
	LPWSTR lpMessageBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMessageBuf, 0, NULL);
	OutputDebugString(lpMessageBuf);
	LocalFree(lpMessageBuf);
}




bool CTools::IsAdmin()
{
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		ShowError();
		return false;
	}

	TOKEN_ELEVATION eve;
	DWORD           len = 0;
	if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == false) {
		ShowError();
		return false;
	}
	CloseHandle(hToken);
	if (len == sizeof(eve)) {
		return eve.TokenIsElevated;
	}
	printf("length of tokeninformation is %d\r\n", len);
	return false;
}

void CTools::WriteStartupDir(const CString& strPath)
{
	TCHAR sPath[MAX_PATH] = _T("");
	GetModuleFileName(NULL, sPath, MAX_PATH);
	int ret = CopyFile(sPath, strPath, FALSE);

	CString strCmd;
	CString file;
	file = strPath;
	strCmd.Format(_T("copy \"%s\" \"%s\""), sPath, file.GetBuffer());

	USES_CONVERSION;
	system(W2A(strCmd.GetBuffer()));
	if (ret == 1) {
		MessageBox(NULL, TEXT("设置开机启动成功！"), TEXT("提示"), NULL);
	}
	//fopen CFile system(copy) CopyFile OpenFile

}

bool CTools::Init()
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