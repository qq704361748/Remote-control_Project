#pragma once
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase{};


typedef int (ThreadFuncBase::* FUNCTYPE)();
class ThreadWorker
{
public:
	ThreadWorker() ;
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f);
	ThreadWorker(const ThreadWorker& worker) ;
	ThreadWorker& operator=(const ThreadWorker& worker) ;


	int operator()();
	bool IsValid() const;

private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};



class CThread
{
public:
	CThread();
	~CThread();
	bool Start();
	bool IsValid();  //true 有效，false表示异常或终止
	bool Stop();
	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker());
	bool IsIdle();  //true 空闲 false表示已分配

protected:
	//返回值小于0则终止线程循环 大于0则警告日志 等于0正常
	//virtual int each_step() = 0;

private:
	void ThreadWorker();
	static void ThreadEntry(void* arg);

private:
	HANDLE m_hThread;
	bool m_bStatus;  //false 表示线程将要关闭，true表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class ThreadPool
{
public:
	ThreadPool(size_t size);
	ThreadPool();
	~ThreadPool();

	bool Invoke();
	void Stop();
	//返回-1 表示分配失败，大于等于0，表示第n个线程进行工作
	int DispatchWorker(const ThreadWorker& worker);
	bool CheckThreadValid(size_t index);
private:
	std::mutex m_lock;
	std::vector<CThread*> m_threads;
};

#include "CThread.inl"
