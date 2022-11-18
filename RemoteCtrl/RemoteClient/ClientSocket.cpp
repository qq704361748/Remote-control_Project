#include "pch.h"
#include "ClientSocket.h"


//ClientSocket server;

CClientSocket* CClientSocket::m_instance = NULL;

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();


PacketData::PacketData(const char* pData, size_t nLen, UINT mode, WPARAM wParam)
{
	strData.resize(nLen);
	memcpy((char*)strData.c_str(), pData, nLen);
	nMode = mode;
	this->wParam = wParam;
}

PacketData::PacketData(const PacketData& data)
{
	strData = data.strData;
	nMode = data.nMode;
	wParam = data.wParam;
}

PacketData& PacketData::operator=(const PacketData& data)
{
	if (this!=&data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	return *this;
}

string GetErrorInfo(int wsaErrCode)
{
	string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
	              FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL,
	              wsaErrCode,
	              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	              (LPTSTR)&lpMsgBuf,
	              0, NULL
	             );
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}


MouseEvent::MouseEvent()
{
	nAction = 0;
	nButton = -1;
	ptXY.x  = 0;
	ptXY.y  = 0;
}

CClientSocket* CClientSocket::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientSocket();
	}
	return m_instance;
}

bool CClientSocket:: InitSocket()
{
	if (m_sock != INVALID_SOCKET) {
		CloseSocket();
	}
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1) {
		return false;
	}
	SOCKADDR_IN serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	
	serv_adr.sin_family           = AF_INET;
	// TRACE("addr %08X nIP %08X\r\n", inet_addr("127.0.0.1"), nIP);
	serv_adr.sin_addr.S_un.S_addr = htonl(m_nIP);
	serv_adr.sin_port             = htons(m_nPort);

	if (serv_adr.sin_addr.S_un.S_addr == INADDR_NONE) {
		AfxMessageBox(TEXT("指定的IP地址不存在！"));
		return false;
	}

	//连接
	int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(SOCKADDR_IN));
	if (ret == -1) {
		AfxMessageBox(TEXT("连接失败"));
		TRACE("连接失败：%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
		return false;
	}

	return true;
}


#define BUFFER_SIZE 4096000

int CClientSocket::DealCommand()
{
	if (m_sock == -1) return -1;
	char* buffer = m_buffer.data();
	if (buffer == NULL)
	{
		TRACE("内存不足! \r\n");
		return -2;
	}
	static size_t index = 0;
	while (true) {
		size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
		if ((len <= 0 ) && (index ==0))return -1;

		index += len;
		len      = index;
		m_packet = CPacket((BYTE*)buffer, len);
		if (len > 0) {
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			return m_packet.sCmd;
		}
	}
	return -1;
}

bool CClientSocket::Send(const char* pData, int nSize)
{
	if (m_sock == -1) return false;
	return send(m_sock, pData, nSize, 0) > 0;
}

bool CClientSocket::Send(const CPacket& pack)
{
	//TRACE("m_sock = %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lparam)
{
	//定义一个消息的数据结构(数据和数据长度,模式)  回调消息的数据结构(hwnd,message)
	HWND hWnd = (HWND)lparam;

	if (InitSocket() == true)
	{
		PACKET_DATA data = *(PacketData*)wParam;
		delete (PacketData*)wParam;

		int ret = send(m_sock, data.strData.c_str(), (int)data.strData.size(), 0);//通过命令调用send处
		if (ret > 0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET)
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || index > 0)
				{
					index += (size_t)length;
					size_t nlen = index;
					CPacket pack((BYTE*)pBuffer, nlen);
					if (nlen > 0)
					{
						TRACE("ack pack %d to hWnd %08X %d %d\r\n", pack.sCmd, hWnd, index, nlen);
						//TRACE("%04X\r\n", *(WORD*)(pBuffer + nlen));
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE)
						{
							CloseSocket();
							return;
						}
					}
					index -= nlen;
					memmove(pBuffer, pBuffer + nlen, index);
				}
				else
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else {
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else
	{
		//TODO:错误处理
		CloseSocket();
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}

bool CClientSocket::GetFilePath(std::string& strPath)
{
	if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) //当命令为2时，即为获取文件目录
	{
		strPath = m_packet.strData;
		return true;
	}
	return false;
}


bool CClientSocket::GetMouseEvent(MOUSEEVENT& mouse)
{
	if (m_packet.sCmd == 5) {
		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEVENT));
		return true;
	}
	return false;
}

CPacket& CClientSocket::GetPacket()
{
	return m_packet;
}

void CClientSocket::CloseSocket()
{
	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
}

void CClientSocket::UpdateAddress(int nIP, int nPort)
{
	if (m_nIP != nIP || m_nPort != nPort)
	
		m_nIP = nIP;
		m_nPort = nPort;
	}
	

/*bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks,bool isAutoClose)
{
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {

		// if(InitSocket() ==false) return false;

		m_hThread=(HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
	}

	m_lock.lock();

	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClose));
	m_lstSend.push_back(pack);

	m_lock.unlock();

	WaitForSingleObject(pack.hEvent, INFINITE);
	std::map < HANDLE, std::list < CPacket >&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it!=m_mapAck.end()) {
		
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	}
	return false;
}*/

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClose,WPARAM wParam)
{
	
	UINT nMode = isAutoClose ? CSM_AUTOCLOSE : 0 ;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	//TRACE("PostThreadMessage Ret: %d\r\n", ret);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

unsigned __stdcall CClientSocket::threadEntry(void* arg)
{
	auto* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

/*void CClientSocket::threadFunc()
{
	
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size()> 0) {
			
			TRACE("m_lstSend.size : %d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if(Send(head) == false) {
				TRACE("发送失败！\r\n");
				continue;
			}

			std::map < HANDLE, std::list < CPacket >&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					if (length > 0 || index > 0) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						

						if (size > 0) {
							//TODO:通知对应的函数进行响应
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);

							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);
						break;
					}
				} while (it0->second == false  );
			}
			

			//std::list<CPacket> lstRecv;
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if(InitSocket() == false) {
				InitSocket();
			}
		} else {
			Sleep(1);
		}
	}
	CloseSocket();
}*/

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		SetEvent(m_eventInvoke);
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);//使用map  将数据发送给对应的发送函数
		}

	}

}


CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, MSGFUNC>::const_iterator it= ss.m_mapFunc.begin();
	for (;it!= ss.m_mapFunc.end();it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket():m_nIP(INADDR_ANY), m_nPort(0),m_sock(INVALID_SOCKET), m_bAutoClose(true),m_hThread(INVALID_HANDLE_VALUE)
{
	//m_sock = INVALID_SOCKET;
	if (InitSockEnv() == FALSE) {

		MessageBox(NULL, TEXT("无法初始化套接字环境,请检查网络设置"), TEXT("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);

	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败了！\r\n");
	}
	CloseHandle(m_eventInvoke);

	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	struct {
		UINT message;
		MSGFUNC func;
	}funcs[] = {
		{WM_SEND_PACK,&CClientSocket::SendPack},
		{0,NULL}
	};

	for (int i = 0; funcs[i].message != 0; i++)//相当于把映射表,插入到map中去;
	{
		if (m_mapFunc.insert(std::pair<UINT, CClientSocket::MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
		{
			TRACE("插入失败\r\n");
		}
	}
	

}

CClientSocket::~CClientSocket()
{
	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
	WSACleanup();
}

BOOL CClientSocket::InitSockEnv()
{
	//初始化SOCKET
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
		return FALSE;
	}
	return TRUE;
}

void CClientSocket::releaseInstance()
{
	if (m_instance != NULL) {
		CClientSocket* tmp = m_instance;
		m_instance         = NULL;
		delete tmp;
	}
}

CClientSocket::CHelper::CHelper()
{
	CClientSocket::getInstance();
}

CClientSocket::CHelper::~CHelper()
{
	CClientSocket::releaseInstance();
}

CPacket::CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}

CPacket::CPacket(WORD nCmd, const BYTE* pData, size_t nSize) //封包
{
	sHead   = 0xFEFF;
	nLength = nSize + 4;
	sCmd    = nCmd;
	if (nSize > 0) {
		strData.resize(nSize);
		memcpy((void*)strData.c_str(), pData, nSize);
	} else {
		strData.clear();
	}

	sSum = 0;
	for (size_t j = 0; j < strData.size(); ++j) {
		sSum += BYTE(strData[j]) & 0xFF;
	}
	
}

CPacket::CPacket(const BYTE* pData, size_t& nSize)//解包
{
	size_t i = 0;
	for (; i < nSize; ++i) {
		if (*(WORD*)(pData + i) == 0xFEFF) {
			sHead = *(WORD*)(pData + i);
			i += 2;
			break;
		}
	}
		if (i + 8 > nSize) {
			//包数据可能不全，或包头未能全部接收
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {
			//包未完全接收到

			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i);
		i += 2;

		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); //-命令2字节 - 和校验2字节
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
	sHead   = pack.sHead;
	nLength = pack.nLength;
	sCmd    = pack.sCmd;
	strData = pack.strData;
	sSum    = pack.sSum;
}

CPacket& CPacket::operator=(const CPacket& pack)
{
	if (this != &pack) {
		sHead   = pack.sHead;
		nLength = pack.nLength;
		sCmd    = pack.sCmd;
		strData = pack.strData;
		sSum    = pack.sSum;
	}
	return *this;
}

int CPacket::Size() //获取包数据大小
{
	return nLength + 6;
}

const char* CPacket::Data(std::string& strOut) const
{
	strOut.resize(nLength + 6);
	BYTE* pData   = (BYTE*)strOut.c_str();
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
