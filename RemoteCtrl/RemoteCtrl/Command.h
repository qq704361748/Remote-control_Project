#pragma once
#include <map>

#include "LockDialog.h"


class CCommand
{
public:
	CCommand();
	~CCommand();
	int ExcuteCommoand(int nCmd);
	static void RunCommand(void* arg, int status);

protected:
	typedef int (CCommand::*CMDFUNC)();    //成员函数指针
	std::map<int, CMDFUNC>  m_mapFunction; //命令映射表
	CLockDialog             dlg;
	unsigned                threadid;

protected:
	int MakeDriverInfo();    //获得磁盘信息
	int MakeDirectoryInfo(); //获取指定文件夹下的信息
	int RunFile();           //运行文件
	int DownloadFile();      //下载文件
	int MouseEvent();        //鼠标事件
	int SendScreen();        //发送屏幕截图
	int LockMachine();       //锁机
	int UnlockMachine();     //解锁
	int DeleteLocalFile();   //删除文件
	int TestConnect();       //连接测试

	static unsigned __stdcall threadLockDlg(void* arg); //子线程执行锁机
	void threadLockDlgMain();
};
