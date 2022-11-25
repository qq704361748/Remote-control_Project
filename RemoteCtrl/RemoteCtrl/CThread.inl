ThreadWorker::ThreadWorker() : thiz(NULL), func(NULL) {}

ThreadWorker::ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) : thiz(obj), func(f) {}

ThreadWorker::ThreadWorker(const ThreadWorker& worker)
{
	thiz = worker.thiz;
	func = worker.func;
}

ThreadWorker& ThreadWorker::operator=(const ThreadWorker& worker)
{
	if (this != &worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	return *this;
}

int ThreadWorker::operator()()
{
	if (thiz) {
		return (thiz->*func)();
	}
	return -1;
}

bool ThreadWorker::IsValid() const
{
	return (thiz != NULL) && (func != NULL);
}

CThread::CThread()
{
	m_hThread = NULL;
	m_bStatus = false;
}

CThread::~CThread()
{
	Stop();
}

bool CThread::Start()
{
	m_bStatus = true;
	m_hThread = (HANDLE)_beginthread(&CThread::ThreadEntry, 0, this);
	if (!IsValid()) {
		m_bStatus = false;
	}
	return m_bStatus;
}

bool CThread::IsValid()
{
	if (m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE) return false;
	return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
}

bool CThread::Stop()
{
	if (m_bStatus == false) {
		return true;
	}
	m_bStatus = false;
	DWORD ret = WaitForSingleObject(m_hThread, 1000);
	if(ret ==WAIT_TIMEOUT) {
		TerminateThread(m_hThread,-1);
	}
	UpdateWorker();
	return ret ==WAIT_OBJECT_0;
}

void CThread::UpdateWorker(const ::ThreadWorker& worker)
{
	if ((m_worker.load() != NULL) && (m_worker.load()!=&worker)) {
		::ThreadWorker* pWorker = m_worker.load();
		m_worker.store(NULL);
		delete pWorker;
	}
	if (m_worker.load() == &worker) return;

	if (!worker.IsValid()) {
		m_worker.store(NULL);
		return;
	}
	
	m_worker.store(new ::ThreadWorker(worker));
}

bool CThread::IsIdle()
{
	if (m_worker.load() == NULL) return true;
	return!m_worker.load()->IsValid();
}


void CThread::ThreadWorker()
{
	while (m_bStatus) {
		if(m_worker.load() == NULL) {
			Sleep(1);
			continue;
		}
		::ThreadWorker worker = *m_worker.load();
		if (worker.IsValid()) {
			if(WaitForSingleObject(m_hThread,0) == WAIT_TIMEOUT) {
				int ret = worker();
				if (ret != 0) {
					CString str;
					str.Format(TEXT("Thread found warning code %d\r\n"), ret);
					OutputDebugString(str);
				}

				if (ret < 0) {
					m_worker.store(NULL);
				}
			}
		}
		else {
			Sleep(1);
		}
	}
}

void CThread::ThreadEntry(void* arg)
{
	auto* thiz = (CThread*)arg;
	if (thiz) {
		thiz->ThreadWorker();
	}
	_endthread();
}

ThreadPool::ThreadPool(size_t size)
{
	m_threads.resize(size);
	for (size_t i = 0; i < size; i++) {
		m_threads[i] = new CThread();
	}
}

ThreadPool::~ThreadPool()
{
	Stop();
	for (size_t i = 0; i < m_threads.size(); i++) {
		delete m_threads[i];
		m_threads[i] = NULL;
	}
	m_threads.clear();
}

bool ThreadPool::Invoke()
{
	bool ret = true;
	for (size_t i = 0; i < m_threads.size(); i++) {
		if (m_threads[i]->Start() == false) {
			ret = false;
			break;
		}
	}
	if (ret = false) {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	return ret;
}

void ThreadPool::Stop()
{
	for (size_t i = 0; i < m_threads.size(); i++) {
		m_threads[i]->Stop();
	}
}

int ThreadPool::DispatchWorker(const ThreadWorker& worker)
{
	int index = -1;
	m_lock.lock();
	for (size_t i = 0; i < m_threads.size(); i++) {
		m_threads[i]->IsIdle();
		m_threads[i]->UpdateWorker(worker);
		index = i;
		break;
	}
	m_lock.unlock();
	return index;
}

bool ThreadPool::CheckThreadValid(size_t index)
{
	if (index < m_threads.size()) {
		return m_threads[index]->IsValid();
	}
	return false;
}


