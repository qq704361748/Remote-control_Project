#include "Queue.hpp"
#include "Queue.hpp"

template <class T>
CQueue<T>::CQueue()
{
	m_lock = false;
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	m_hThread = INVALID_HANDLE_VALUE;
	if (m_hCompletionPort != NULL) {
		m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
	}
}

template <class T>
CQueue<T>::~CQueue()
{
	if (m_lock) return;
	m_lock = true;
	PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	WaitForSingleObject(m_hThread, INFINITE);
	if (m_hCompletionPort !=NULL) {
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}
}

template <class T>
bool CQueue<T>::PushBack(const T& data)
{
	IocpParam* pParam = new IocpParam(QPush, data);
	if (m_lock) {
		delete pParam;
		return false;
	}

	bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);

	if (ret == false) delete pParam;
	return ret;
}

template <class T>
bool CQueue<T>::PopFront(T& data)
{
	HANDLE    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	IocpParam Param(QPop, data, hEvent);

	if (m_lock) {
		if (hEvent) CloseHandle(hEvent);
		return false;
	}

	bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);

	if (ret == false) {
		CloseHandle(hEvent);
		return false;
	}
	ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
	if (ret) {
		data = Param.Data;
	}
	return ret;
}

template <class T>
size_t CQueue<T>::Size()
{
	HANDLE    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	IocpParam Param(Qsize, T(), hEvent);
	if (m_lock) {
		if (hEvent) CloseHandle(hEvent);
		return -1;
	}

	bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);

	if (ret == false) {
		CloseHandle(hEvent);
		return -1;
	}
	ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
	if (ret) {
		return Param.nOperator;
	}
	return -1;
}

template <class T>
bool CQueue<T>::Clear()
{
	if (m_lock) return false;
	IocpParam* pParam = new IocpParam(QClear, T());

	bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);

	if (ret == false) delete pParam;
	return ret;
}

template <class T>
void CQueue<T>::threadEntry(void* arg)
{
	auto* thiz = (CQueue<T>*)arg;
	thiz->threadMain();
	_endthread();
}

template <class T>
void CQueue<T>::threadMain()
{
	DWORD       dwTransferred = 0;
	PPARAM* pParam = NULL;
	ULONG_PTR   CompletionKey = 0;
	OVERLAPPED* pOverlapped = NULL;

	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {

		if (dwTransferred == 0 || CompletionKey == 0) {
			printf("thread is prepare to exit!\r\n");
			break;
		}

		pParam = (PPARAM*)CompletionKey;
		DealParam(pParam);

	}
	while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
		if (dwTransferred == 0 || CompletionKey == 0) {
			printf("thread is prepare to exit!\r\n");
			continue;
		}
		pParam = (PPARAM*)CompletionKey;
		DealParam(pParam);
	}
	HANDLE hTemp = m_hCompletionPort;
	m_hCompletionPort = NULL;
	CloseHandle(hTemp);
	
}

template <class T>
void CQueue<T>::DealParam(PPARAM* pParam)
{
	switch (pParam->nOperator) {

	case QPush:
	{
		m_lstData.push_back(pParam->Data);
		delete pParam;
	}
	break;
	case QPop:
	{
		if (m_lstData.size() > 0) {
			pParam->Data = m_lstData.front();
			m_lstData.pop_front();
		}
		if (pParam->hEvent != NULL) {
			SetEvent(pParam->hEvent);
		}
	}
	break;
	case Qsize:
	{
		pParam->nOperator = m_lstData.size();
		if (pParam->hEvent != NULL) {
			SetEvent(pParam->hEvent);
		}
	}
	break;
	case QClear:
	{
		m_lstData.clear();
		delete pParam;
	}
	break;
	default:
		OutputDebugString(TEXT("unknown operator!\r\n"));
	}
}

template<class T>
inline KSendQueue<T>::KSendQueue(ThreadFuncBase* obj, KCALLBACK callback)
:CQueue<T>(),m_base(obj),m_callback(callback)
{
	m_thread.Start();
	m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE) & KSendQueue<T>::threadTick));
}

template <class T>
KSendQueue<T>::~KSendQueue()
{
	m_base = NULL;
	m_callback = NULL;
	m_thread.Stop();
}


template <class T>
bool KSendQueue<T>::PopFront(T& data)
{
	return false;
}

template <class T>
bool KSendQueue<T>::PopFront()
{
	typename CQueue<T>::IocpParam* Param = new typename CQueue<T>::IocpParam(CQueue<T>::QPop, T());

	if (CQueue<T>::m_lock) {
		delete Param;
		return false;
	}

	bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);

	if (ret == false) {
		delete Param;
		return false;
	}
	return ret;
}

template <class T>
int KSendQueue<T>::threadTick()
{
	if (WaitForSingleObject(CQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) return 0;
	if(CQueue<T>::m_lstData.size()>0 ) {
		PopFront();
	}
	//Sleep(1);
	return 0;

}

template <class T>
void KSendQueue<T>::DealParam(typename CQueue<T>::PPARAM* pParam)
{
	switch (pParam->nOperator) {

	case CQueue<T>::QPush:
	{
		CQueue<T>::m_lstData.push_back(pParam->Data);
		delete pParam;
	}
	break;
	case CQueue<T>::QPop:
	{
		if (CQueue<T>::m_lstData.size() > 0) {
			pParam->Data = CQueue<T>::m_lstData.front();
			if((m_base->*m_callback)(pParam->Data) == 0)
				CQueue<T>::m_lstData.pop_front();
		}
		delete pParam;
	}
	break;
	case CQueue<T>::Qsize:
	{
		pParam->nOperator = CQueue<T>::m_lstData.size();
		if (pParam->hEvent != NULL) {
			SetEvent(pParam->hEvent);
		}
	}
	break;
	case CQueue<T>::QClear:
	{
		CQueue<T>::m_lstData.clear();
		delete pParam;
	}
	break;
	default:
		OutputDebugString(TEXT("unknown operator!\r\n"));
	}



}

