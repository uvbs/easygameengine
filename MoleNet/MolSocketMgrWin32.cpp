#include "MolSocketMgrWin32.h"

#include "MolSocket.h"

#include "liblzo/minilzo.h"

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);

initialiseSingleton(SocketMgr);

/** 
 * ���캯��
 */
SocketMgr::SocketMgr()
: m_maxsockets(0),uncombuf(NULL),combuf(NULL)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,0),&wsaData);

	m_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,(ULONG_PTR)0,0);

    if (lzo_init() != LZO_E_OK)
    {
		printf("lzo��ʼʧ�ܡ�");
    }

	combuf = (unsigned char*)malloc(MOL_REV_BUFFER_SIZE);
	uncombuf = (unsigned char*)malloc(MOL_REV_BUFFER_SIZE);
}

/** 
 * ��������
 */
SocketMgr::~SocketMgr()
{
	ClearMesList();

	ClearTasks();

	if(combuf) free(combuf);
	combuf = NULL;
	if(uncombuf) free(uncombuf);
	uncombuf = NULL;
}

void SocketMgr::SpawnWorkerThreads()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	threadcount = si.dwNumberOfProcessors;

	printf("IOCP: ���� %u �������̡߳�\n",threadcount);
	for(long x = 0; x < threadcount; ++x)
		ThreadPool.ExecuteTask(new SocketWorkerThread());
}

/**
 * ���ϵͳ�����е�����
 */
void SocketMgr::ClearTasks(void)
{
	if(_ThreadBases.empty()) return;

	std::list<ThreadBase*>::iterator iter = _ThreadBases.begin();
	for(;iter!=_ThreadBases.end();++iter)
		if((*iter)) delete (*iter);
	_ThreadBases.clear();
}

bool SocketWorkerThread::run()
{
	try
	{
		HANDLE cp = sSocketMgr.GetCompletionPort();
		DWORD len;
		Socket * s;
		OverlappedStruct * ov;
		LPOVERLAPPED ol_ptr;

		while(true)
		{
			if(!GetQueuedCompletionStatus(cp,&len,(PULONG_PTR)&s,&ol_ptr,10000))
				continue;

			ov = CONTAINING_RECORD(ol_ptr,OverlappedStruct,m_overlap);

			if(ov->m_event == SOCKET_IO_THREAD_SHUTDOWN)
			{
				delete ov;
				return true;
			}

			if(ov->m_event < NUM_SOCKET_IO_EVENTS)
				ophandlers[ov->m_event](s,len);
		}
	}
	catch (std::exception e)
	{
		printf("�����߳��쳣:%s\n",e.what());
	}

	return true;
}

void HandleReadComplete(Socket * s,uint32 len)
{
	if(!s->IsDeleted())
	{
		s->m_readEvent.Unmark();
		if(len)
		{
			s->GetReadBuffer().IncrementWritten(len);
			s->OnRead(len);
			s->SetupReadEvent();
		}
		else 
			s->Delete();
	}
}

void HandleWriteComplete(Socket * s,uint32 len)
{
	if(!s->IsDeleted())
	{
		s->m_writeEvent.Unmark();
		s->BurstBegin();
		s->GetWriteBuffer().Remove(len);
		if(s->GetWriteBuffer().GetContiguiousBytes() > 0)
			s->WriteCallback();
		else
			s->DecSendLock();
		s->BurstEnd();
	}
}

void HandleShutdown(Socket * s,uint32 len)
{

}

void SocketMgr::CloseAll()
{
	std::list<Socket*> tokill;

	socketLock.Acquire();
	for(std::map<SOCKET,Socket*>::iterator itr = _sockets.begin(); itr != _sockets.end(); ++itr)
		tokill.push_back((*itr).second);
	socketLock.Release();

	for(std::list<Socket*>::iterator itr = tokill.begin(); itr != tokill.end(); ++itr)
	{
		if((*itr))
			(*itr)->Disconnect();
	}

	size_t size;
	do
	{
		socketLock.Acquire();
		size = _sockets.size();
		socketLock.Release();
	}while(size);
}

void SocketMgr::ShutdownThreads()
{
	for(int i = 0; i < threadcount; ++i)
	{
		OverlappedStruct * ov = new OverlappedStruct(SOCKET_IO_THREAD_SHUTDOWN);
		PostQueuedCompletionStatus(m_completionPort, 0, (ULONG_PTR)0, &ov->m_overlap);
	}
}

void SocketMgr::ShowStatus()
{
	socketLock.Acquire();

	printf("_sockets.size(): %d", _sockets.size());

	socketLock.Release();
}

/**
 * �����Ϣ�б�
 */
void SocketMgr::ClearMesList(void)
{
	if(_MesList.empty()) return;

	mesLock.Acquire();
	std::list<MessageStru>::iterator iter = _MesList.begin();
	for(;iter!=_MesList.end();++iter)
	{
		if((*iter).mes)
			delete (*iter).mes;
	}
	_MesList.clear();
	mesLock.Release();
}

/** 
* ѹ������Ҫ���������
*
* @param out ����Ҫѹ��������
* @param declength ѹ��������ݳ���
*
* @return ����ѹ���������
*/
char* SocketMgr::compress(CMolMessageOut &out,int *declength)
{
	if(out.getData() == NULL || out.getLength() <= 0) return NULL;

	int length = out.getLength();
	lzo_uint out_len = length;
	int result=0;

	memset(combuf,0,MOL_REV_BUFFER_SIZE);

	memcpy(combuf,out.getData(),length);

	// ��������
	Encrypto((unsigned char*)combuf,length);

	//result = lzo1x_1_compress((unsigned char*)out.getData(),length,combuf,&out_len,wrkmem);

	//   if(result == LZO_E_OK) 
	//{

	//   }
	//else 
	//{
	//       LOG_ERRORR("lzoѹ���������⡣");
	//   }

	*declength = length;

	return (char*)combuf;
}

/**
* ��ѹ���ǽ��յ�������
*
* @param data Ҫ��ѹ������
* @param srclength ��ѹ�����ݵĳ���
* @param declength ��ѹ������ݵĳ���
*
* @return ���ؽ�ѹ�������
*/
char* SocketMgr::uncompress(unsigned char *data,int srclength,int *declength)
{
	if(data == NULL || srclength <= 0) return NULL;

	int src_length = srclength;
	lzo_uint in_len;
	int result=0;

	//src_length = src_length * 200;

	memset(uncombuf,0,MOL_REV_BUFFER_SIZE);

	memcpy(uncombuf,data,srclength);

	// ��������
	Decrypto((unsigned char*)uncombuf,srclength);

	//   result = lzo1x_decompress(data,srclength,uncombuf,&in_len,NULL);

	//   if(result == LZO_E_OK) 
	//{
	//      //ɾ��ԭ��������
	//      //if(data) free(data);
	//      //  data = NULL;
	//   }
	//else 
	//{
	//      LOG_ERRORR("lzo��ѹ�������⡣");
	//   }

	*declength = srclength;
	return (char*)uncombuf;
}

/**
* ��������
*
* @param data Ҫ���ܵ�����
* @param length Ҫ���ܵ����ݵĳ���
*/
void SocketMgr::Encrypto(unsigned char *data,unsigned long length)
{
	if(data == NULL || length <= 0) return;

	unsigned char pKeyList[] = {76,225,112,120,103,92,84,105,8,12,238,122,206,165,222,21,117,217,106,214,239,66,32,3,85,67,224,180,
		240,233,236,171,89,13,52,109,123,99,132,213,15,137,226,69,231,228,60,28,190,193,74,144,81,53,17,101,230,207,79,93,88,36,30,
		141,115,110,20,169,173,243,219,80,72,184,125,175,174,139,95,24,148,48,113,182,50,223,61,118,140,14,78,181,16,4,121,73,187,
		131,38,34,136,221,191,204,245,246,247,248,249,250,251,252,253,254,255};

	for(int i=0;i<(int)length;i++)
	{
		data[i] = pKeyList[data[i]];
	}
}

/**
* ��������
*
* @param data Ҫ���ܵ�����
* @param length Ҫ���ܵ����ݵĳ���
*/
void SocketMgr::Decrypto(unsigned char *data,unsigned long length)
{
	if(data == NULL || length <= 0) return;

	unsigned char pKeyList[] = {123,182,156,23,93,178,119,145,8,99,167,168,9,33,89,40,92,54,189,236,66,15,124,101,79,155,133,128,47,231,
		249,250,251,252,253,254,255};

	for(int i=0;i<(int)length;i++)
	{
		data[i] = pKeyList[data[i]];
	}
}