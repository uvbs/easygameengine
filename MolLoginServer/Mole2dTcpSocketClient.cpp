#include "stdafx.h"

#include "Mole2dTcpSocketClient.h"

#include "Mole2dCircularBuffer.h"
#include "MolServerDlg.h"

//#include "lzo/minilzo.h"

//#define HEAP_ALLOC(var,size) \
//    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]
//
//static HEAP_ALLOC(wrkmem,LZO1X_1_MEM_COMPRESS);

#pragma pack(push, 1)
typedef struct
{
	unsigned short opcode;
	unsigned int size;
}logonpacket;
#pragma pack(pop)

BEGIN_MESSAGE_MAP(CMolTcpSocketClient, CWnd)
	ON_MESSAGE(IDD_WM_SOCKET_MESSAGE,&CMolTcpSocketClient::OnSocketNotifyMessage)
END_MESSAGE_MAP()
	
/** 
 * ���캯��
 *
 * @param pParent ��ǰ�ؼ��ĸ��ؼ�
 */
CMolTcpSocketClient::CMolTcpSocketClient(CWnd *pParent)
: m_Parent(pParent),m_Socket(NULL),m_bConnectState(NOCONNECT),
  m_ReadBuffer(NULL),m_BaseFrame(NULL),uncombuf(NULL),combuf(NULL),remaining(0),opcode(0)
{
	if(!GetSafeHwnd())
		CreateSocketHwnd(m_Parent);

	m_ReadBuffer = new CMolCircularBuffer();

	m_ReadBuffer->Allocate(REV_SIZE);

     //if (lzo_init() != LZO_E_OK)
     //{
     //   LOG_ERRORR("lzo��ʼʧ�ܡ�");
     //}

	 combuf = (unsigned char*)malloc(MOL_REV_BUFFER_SIZE);
	 uncombuf = (unsigned char*)malloc(MOL_REV_BUFFER_SIZE);
}

/** 
 * ��������
 */
CMolTcpSocketClient::~CMolTcpSocketClient(void)
{
	// ��������
	CloseConnect();

	delete m_ReadBuffer;
	m_ReadBuffer = NULL;

	if(combuf) free(combuf);
	combuf = NULL;
	if(uncombuf) free(uncombuf);
	uncombuf = NULL;
}

/** 
 * ����ָ���ķ�����
 *
 * @param ipaddress Ҫ���ӵķ�����IP��ַ
 * @param port Ҫ���ӵķ������˿�
 *
 * @return ������������ӳɹ�������,���򷵻ؼ�
 */
bool CMolTcpSocketClient::Connect(std::string ipaddress,int port)
{
	if(ipaddress.empty())
		return false;

	long dwServerIP = inet_addr(ipaddress.c_str());
	if(dwServerIP == INADDR_NONE)
	{
		LPHOSTENT lpHost = ::gethostbyname(ipaddress.c_str());
		if(lpHost != NULL) return false;

		dwServerIP = ((LPIN_ADDR)(lpHost->h_addr))->s_addr;
	}

	// ���winsock����û�н����Ļ�����������
	if(!GetSafeHwnd())
		CreateSocketHwnd(m_Parent);

	m_Socket = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(::WSAAsyncSelect(m_Socket,GetSafeHwnd(),IDD_WM_SOCKET_MESSAGE,FD_READ|FD_CONNECT|FD_CLOSE) == SOCKET_ERROR)
	{
		LOG_ERRORR("WSAAsyncSelect ����ʧ��!");
		return false;
	}

	sockaddr_in SocketAddr;
	SocketAddr.sin_family = AF_INET;
	SocketAddr.sin_port = htons(port);
	SocketAddr.sin_addr.S_un.S_addr = dwServerIP;

	if((::connect(m_Socket,(const sockaddr*)(&SocketAddr),sizeof(SocketAddr)) == SOCKET_ERROR) &&
		(::WSAGetLastError() != WSAEWOULDBLOCK))
	{
		LOG_ERRORR("������:" << ipaddress << "����ʧ��!");
		return false;
	}

	m_bConnectState = CONNECTTING;

	return true;
}

/** 
 * ����socket�Ի���
 *
 * @param pParent ��ǰ���ڵĸ�����
 */
void CMolTcpSocketClient::CreateSocketHwnd(CWnd *pParnet)
{
	if(pParnet == NULL &&
		AfxGetApp())
	{
		pParnet = AfxGetApp()->GetMainWnd();
	}

	if(!CreateEx(NULL,NULL,"",WS_CHILD,CRect(0,0,0,0),pParnet,IDD_SOCKET_WND))
		LOG_ERRORR("WinSocket���ڽ���ʧ��");
}

/** 
 * �ر�����
 */
void CMolTcpSocketClient::CloseConnect(void)
{
	bool bClose = (m_Socket != INVALID_SOCKET);
	m_bConnectState = NOCONNECT;
	if(m_Socket != INVALID_SOCKET)
	{
		::WSAAsyncSelect(m_Socket,GetSafeHwnd(),IDD_WM_SOCKET_MESSAGE,0);
		::closesocket(m_Socket);

		m_Socket = INVALID_SOCKET;
	}
}

/** 
 * ��������
 *
 * @param msg Ҫ���͵�����
 *
 * @return �������ݷ��͵��������������ȫ�����ͳɹ��������ݵĴ�С�����򷵻�SOCKET_ERROR
 */
int CMolTcpSocketClient::Send(CMolMessageOut &msg)
{
	if(msg.getData() == NULL ||
		msg.getLength() <= 0)
		return SOCKET_ERROR;

	if(m_Socket == INVALID_SOCKET) return SOCKET_ERROR;

	int iSendCount = 0;
	int uSended,uSendSize,iErrorCode;
		
	uSended = iErrorCode = 0;
	char *sdata = compress(msg,&uSendSize);

	try
	{
		do 
		{
			iErrorCode = ::send(m_Socket,(sdata+uSended),uSendSize-uSended,0);
			if(iErrorCode == SOCKET_ERROR)
			{
				if(::WSAGetLastError() == WSAEWOULDBLOCK)
				{
					if(iSendCount++ > 100)
						return SOCKET_ERROR;
					else
					{
						Sleep(10);
						continue;
					}
				}
				else
				{
					return SOCKET_ERROR;
				}
			}

			uSended += iErrorCode;
			iSendCount = 0;
		} while (uSended < uSendSize);
	}
	catch(...)
	{
		LOG_ERRORR("winsockt�������ݳ��ִ���!");
	}

	return uSendSize;
}

/** 
 * ����socket������Ϣ
 *
 * @param wParam,lParam Ҫ������������Ϣ
 *
 * @return ���ش������
 */
LRESULT CMolTcpSocketClient::OnSocketNotifyMessage(WPARAM wParam,LPARAM lParam)
{
	CMolMessageIn in;

	switch(WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:                      // �����ͻ�������
		{
			UINT uErrorCode = WSAGETSELECTERROR(lParam);

			if(uErrorCode == 0)
				m_bConnectState = CONNECTED;
			else
				CloseConnect();

			return 0;
		}
		break;
	case FD_READ:                         // �����յ�����
		{
			int iLen = ::recv(m_Socket,(char*)(m_ReadBuffer->GetBuffer()),(int)m_ReadBuffer->GetSpace(),0);
			if(iLen == SOCKET_ERROR)
			{
				CloseConnect();
				return 0;
			}

			try
			{
				if(iLen != 0xFFFFFFFF)
					m_ReadBuffer->IncrementWritten(iLen);

				while(true)
				{
					if(!remaining)
					{
						if(m_ReadBuffer->GetSize() < sizeof(logonpacket)) 
							return 0;	

						// ����ȡ�ð汾��
						m_ReadBuffer->Read((unsigned char*)&opcode,sizeof(unsigned short));

						if(opcode != 100) 
							return 0;

						// ����ȡ�ð�ͷ
						m_ReadBuffer->Read((unsigned char*)&remaining,sizeof(unsigned int));
					}

					if(m_ReadBuffer->GetSize() < remaining)
						return 0;

                    m_bConnectState = MESPROCESS;
                    memset(buffer,0,MOL_REV_BUFFER_SIZE);
                   
                    // ȡ��ʵ�����ݰ�
                    m_ReadBuffer->Read((unsigned char*)buffer,remaining);

                    int dlength = 0;
                    char* rdata = uncompress((unsigned char*)buffer,remaining,&dlength);

					remaining = 0;
					opcode = 0;
				}
			}
			catch (...)
			{
				CloseConnect();
			}

			return 0;
		}
		break;
	case FD_CLOSE:                        // �������ӹر�
		{
			CloseConnect();

			return 0;
		}
		break;
	default:
		break;
	}
	return 0;
}

/** 
 * ѹ������Ҫ���������
 *
 * @param out ����Ҫѹ��������
 * @param declength ѹ��������ݳ���
 *
 * @return ����ѹ���������
 */
char* CMolTcpSocketClient::compress(CMolMessageOut &out,int *declength)
{
	if(out.getData() == NULL || out.getLength() <= 0) return NULL;

	int length = out.getLength();
//	lzo_uint out_len = length;
	int result=0;

    memset(combuf,0,MOL_REV_BUFFER_SIZE);

    logonpacket header;
	header.opcode = 100;
	header.size = length;

    // �ȿ�����ͷ
    memcpy(combuf,&header,sizeof(logonpacket));

    // ��������
    Encrypto((unsigned char*)out.getData(),length);

    // ����ʵ�ʵ�����
    memcpy(combuf+sizeof(logonpacket),out.getData(),length);

	//result = lzo1x_1_compress((unsigned char*)out.getData(),length,combuf,&out_len,wrkmem);

 //   if(result == LZO_E_OK) 
	//{

 //   }
	//else 
	//{
 //       LOG_ERRORR("lzoѹ���������⡣");
 //   }

	*declength = length+sizeof(logonpacket);

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
char* CMolTcpSocketClient::uncompress(unsigned char *data,int srclength,int *declength)
{
	if(data == NULL || srclength <= 0) return NULL;

     int src_length = srclength;
//     lzo_uint in_len;
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
void CMolTcpSocketClient::Encrypto(unsigned char *data,unsigned long length)
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
void CMolTcpSocketClient::Decrypto(unsigned char *data,unsigned long length)
{
	if(data == NULL || length <= 0) return;

	unsigned char pKeyList[] = {123,182,156,23,93,178,119,145,8,99,167,168,9,33,89,40,92,54,189,236,66,15,124,101,79,155,133,128,47,231,
		249,250,251,252,253,254,255};

	for(int i=0;i<(int)length;i++)
	{
		data[i] = pKeyList[data[i]];
	}
}