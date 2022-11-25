#pragma once
#include <list>
#include <atomic>
#include "pch.h"
#include "CThread.hpp"

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
	virtual ~CQueue();
	bool   PushBack(const T& data);
	virtual bool   PopFront(T& data);
	size_t Size();
	bool   Clear();
protected:
	static void threadEntry(void* arg);
	virtual void        threadMain();
	virtual void        DealParam(PPARAM* pParam);
protected:
	std::list<T>      m_lstData;
	HANDLE            m_hCompletionPort;
	HANDLE            m_hThread;
	std::atomic<bool> m_lock;
};



template<class T>
class KSendQueue:public CQueue<T>,ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* KCALLBACK)(T& data);

	KSendQueue(ThreadFuncBase* obj, KCALLBACK callback);

	virtual ~KSendQueue();
protected:
	virtual bool   PopFront(T& data);
	bool PopFront();
	int threadTick();
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam);

private:
	ThreadFuncBase* m_base;
	KCALLBACK m_callback;
	CThread m_thread;
};

typedef KSendQueue<std::vector<char>>::KCALLBACK SENDCALLBACK;
#include "Queue.inl"
