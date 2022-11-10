#pragma once

#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket();                                           //默认构造
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize); //封包
	CPacket(const BYTE* pData, size_t& nSize);           //解包
	CPacket(const CPacket& pack);                        //拷贝构造
	CPacket& operator=(const CPacket& pack);             //赋值构造

	int         Size(); //获取包数据大小
	const char* Data(); //获取包数据

	~CPacket() = default;

	WORD        sHead;   //包头（固定位 FE FF）
	DWORD       nLength; //包长（从命令开始到和校验结束）
	WORD        sCmd;    //命令
	std::string strData; //数据
	WORD        sSum;    //和校验
	std::string strOut;  //整个包的数据
};

#pragma pack(pop)


typedef struct MouseEvent
{
	MouseEvent();
	WORD  nAction; //点击、移动、双击
	WORD  nButton; //左键、右键、中键
	POINT ptXY;    //坐标
}         MOUSEEVENT, * PMOUSEEVENT;

typedef struct file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvalid;   //是否有效
	BOOL IsDirectory; //是否为目录 0否，1是
	BOOL HasNext;     //是否还有后续 0没有，1有
	char szFileName[256];
}        FILEINFO, * PFILEINFO;