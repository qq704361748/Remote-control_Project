#include "pch.h"
#include "Packet.h"

CPacket::CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}

CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize) //封包
{
	sHead = 0xFEFF;
	nLength = nSize + 4;
	sCmd = nCmd;
	if (nSize > 0) {
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	}
	else {
		strData.clear();
	}

	sSum = 0;
	for (size_t j = 0; j < strData.size(); ++j) {
		sSum += BYTE(strData[j]) & 0xFF;
	}
}

CPacket::CPacket(const BYTE* pData, size_t& nSize) //解包
{
	size_t i = 0;
	for (; i < nSize; ++i) {
		if (*(WORD*)(pData + i) == 0xFEFF) {
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}
	if (i + 8 > nSize) //包数据可能不全，或包头未能全部接收
	{
		nSize = 0;
		return;
	}

	nLength = *(DWORD*)(pData + i);
	i += 4;
	if (nLength + i > nSize) //包未完全接收到
	{
		nSize = 0;
		return;
	}
	sCmd = *(WORD*)(pData + i);
	i += 2;

	if (nLength > 4) {
		strData.resize(nLength - 2 - 2);  //-命令2字节 - 和校验2字节
		memcpy((void*)strData.c_str(), pData + i, nLength - 4);
		i += nLength - 4;
	}

	sSum = *(WORD*)(pData + i);
	i += 2;
	WORD sum = 0;
	for (size_t j = 0; j < strData.size(); j++) {
		sum += BYTE(strData[j]) & 0xFF;
		//sum = BYTE(strData[j]) & 0xFF + sum;
	}

	if (sum == sSum) {
		nSize = i;
		return;
	}
	nSize = 0;

}


CPacket::CPacket(const CPacket& pack)
{
	sHead = pack.sHead;
	nLength = pack.nLength;
	sCmd = pack.sCmd;
	strData = pack.strData;
	sSum = pack.sSum;
}

CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	return *this;
}

int CPacket::Size() //获取包数据大小
{
	return nLength + 6;
}

const char* CPacket::Data()
{
	strOut.resize(nLength + 6);
	BYTE* pData = (BYTE*)strOut.c_str();
	*(WORD*)pData = sHead;
	pData += 2;
	*(DWORD*)pData = nLength;
	pData += 4;
	*(WORD*)pData = sCmd;
	pData += 2;
	memcpy(pData, strData.c_str(), strData.size());
	pData += strData.size();
	*(WORD*)pData = sSum;

	return strOut.c_str();
}

