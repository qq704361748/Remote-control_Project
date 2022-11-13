#pragma once
#include <string>
#include <Windows.h>
#include <atlimage.h>
class CTools
{
public:
	static void Dump(BYTE* pData, size_t nSize); //输出磁盘信息

	static int Bytes2Image(CImage& image, const std::string& strData);

};
