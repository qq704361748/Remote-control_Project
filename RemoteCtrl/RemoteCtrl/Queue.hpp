#pragma once
#include <list>
#include <atomic>
#include "pch.h"

template <class T>
class CQueue
{
public:
	enum
	{
		QNone,
		QPush,
		QPop,
		Qsize,
		QClear
	};


	typedef struct IocpParam
	{
		size_t                 nOperator;
		T                      Data;
		HANDLE                 hEvent; //pop操作需要
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data      = data;
			hEvent    = hEve;
		}


		IocpParam()
		{
			nOperator = QNone;
		}
	} PPARAM; //Post Parameter 用于投递信息的结构体

public:
	CQueue();
	~CQueue();
	bool   PushBack(const T& data);
	bool   PopFront(T& data);
	size_t Size();
	bool   Clear();
private:
	static void threadEntry(void* arg);
	void        threadMain();
	void        DealParam(PPARAM* pParam);
private:
	std::list<T>      m_lstData;
	HANDLE            m_hCompletionPort;
	HANDLE            m_hThread;
	std::atomic<bool> m_lock;
};


#include "Queue.inl"
