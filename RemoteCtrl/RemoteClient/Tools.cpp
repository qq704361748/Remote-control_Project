#include "pch.h"
#include "Tools.h"


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

int CTools::Bytes2Image(CImage& image, const std::string& strData)
{
	BYTE* pData = (BYTE*)strData.c_str();
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	if (hMem == NULL)
	{
		TRACE("内存不足!");
		Sleep(10);
		return -1;
	}
	IStream* pStream = NULL;
	HRESULT hRet = CreateStreamOnHGlobal(hMem, true, &pStream);
	if (hRet == S_OK)
	{
		ULONG length = 0;
		pStream->Write(pData, strData.size(), &length);
		LARGE_INTEGER bg = { 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		if ((HBITMAP)image != NULL)
		{
			image.Destroy();
		}
		image.Load(pStream);
	}

}

