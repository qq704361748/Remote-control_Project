#pragma once
#include <map>

#include "LockDialog.h"
#include "ServerSocket.h"


class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommoand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

protected:
	typedef int (CCommand::*CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);    //成员函数指针
	std::map<int, CMDFUNC>  m_mapFunction; //命令映射表
	CLockDialog             dlg;
	unsigned                threadid;

protected:
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);    //获得磁盘信息
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket); //获取指定文件夹下的信息
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);           //运行文件
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);      //下载文件
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);        //鼠标事件
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);        //发送屏幕截图
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);       //锁机
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);     //解锁
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);   //删除文件
	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket);       //连接测试

	static unsigned __stdcall threadLockDlg(void* arg); //子线程执行锁机
	void threadLockDlgMain();
};
