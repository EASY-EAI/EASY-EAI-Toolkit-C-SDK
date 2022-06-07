#include "ipcClient.h"

typedef struct{
	IPCClient *pSelf;	
}IPCClient_para_t;

/* ========================================= Thread ============================================ */
void *readDataFromServerThread(void *para)
{
    // 进程间通信对象
	IPCClient_para_t *pIPCPara = (IPCClient_para_t *)para;
    IPCClient *pSelf = pIPCPara->pSelf;

    IPC_MSG_t msg = {0};
    int32_t ret;
    while(1){
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }    
        // 进程间通信通道 未准备就绪
        if(0 > pSelf->mSocketFd) {
            usleep(10*1000);
    	    continue;
        }
        
        ret = 0;
        ret = tcp_recv(pSelf->mSocketFd, &msg, sizeof(msg));
        /* 此处应做容错处理
        *  1. ret < 0 (需要把缓冲区清空,并通知服务器重发--怎么重发？)
        *  2. msg.msgHeader不对(需要把缓冲区清空,并通知服务器重发--怎么重发？)
        *  3. ret ≠ sizeof(msg)
        */

        msg.payload = NULL;
        msg.payload = malloc(msg.msgLen);
        if(msg.payload){
            // 取出payload
            ret = tcp_recv(pSelf->mSocketFd, msg.payload, msg.msgLen);
            if(-1 != ret){
                // 把payload写入回调
                if((pSelf->mpCliParentObj)&&(pSelf->mpCliCBFunc)){
                    pSelf->mpCliCBFunc(pSelf->mpCliParentObj, &msg);
                }
            }
            
            free(msg.payload);
        }
    }
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

void *writeDataToServerThread(void *para)
{
    // 进程间通信对象
	IPCClient_para_t *pIPCPara = (IPCClient_para_t *)para;
    IPCClient *pSelf = pIPCPara->pSelf;

    IPC_MSG_t msg;
    while(1){
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }
        // 进程间通信通道 未准备就绪
        if(0 > pSelf->mSocketFd) {
            usleep(10*1000);
    	    continue;
        }
        // 写队列为空
        if(pSelf->mMsgQueue.empty()){
            usleep(100*1000);
            continue;
        }
    
        // 从写队列里取节点
        memset(&msg, 0, sizeof(msg));
        msg = pSelf->mMsgQueue.front();

        //printf("[IPCClient](%d): ---Send data--- \n", msg.srcClientId);
        // 数据写入Socket  
        //printf("============ ProcessId[%d] Send Data(Type:[%d] - Size:[%d]) To ProcessId[%d]\n", msg.srcClientId, msg.msgType, msg.msgLen, msg.dstClientId);
        tcp_send(pSelf->mSocketFd, &msg, sizeof(msg));
        tcp_send(pSelf->mSocketFd, msg.payload, msg.msgLen);

        if(msg.payload)
            free(msg.payload);
        
        pSelf->mMsgQueue.pop();

    }
    
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
    pthread_exit(NULL);
}

/* =========================================== IPC =============================================*/
IPCClient *IPCClient::m_pSelf = NULL;
IPCClient::IPCClient() :
    mSocketFd(-1),
    mpCliParentObj(NULL),
    mpCliCBFunc(NULL),
    mClientId(0),
	mbObjIsInited(false)
{

}

IPCClient::~IPCClient()
{
    unInit();
}

void IPCClient::createIPCClient()
{
    if(m_pSelf == NULL){
        m_pSelf = new IPCClient();
   }
}

void IPCClient::init(int32_t srvPort, int32_t cliId)
{
    mClientId = cliId;

#if USING_INET
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(srvPort);
#else
    struct sockaddr_un server;
	memset(&server, 0, sizeof(server));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, AF_UNIX_IPC_SERV);
#endif
    /* 说明:
     * sockaddr_un：
     *   UNIX域套接字，仅可以用在“服务器和客户端在同一台机器”上的情况，通常用于进程间通信
     * sockaddr/sockaddr_in：
     *   这两个结构体是同一个东西，指针可以互相强转，只是sockaddr_in的成员写得更加明确
     */
     
#if USING_INET
    if((mSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return ;
    }
#else
    if((mSocketFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        return ;
    }
#endif
    /* 说明:
     * int socket(int domain, int type, int protocal)
     *  【domain】：
     *      IPV4：AF_INET 
     *      IPV6：AF_INET6 
     *      本地：AF_LOCAL/AF_UNIX
     *  【type】
     *      SOCK_STREAM: 字节流   ---TCP(包与包之间没有边界，需要应用层定义)
     *      SOCK_DGRAM: 数据报 ---UDP
     *      SOCK_RAW: 原始套接字
     *  【protocal】
     *      原本是用来指定通信协议的，但现在基本废弃。目前通常填0
     */
     
#if USING_INET
    char ipAddr[16] = "127.0.0.1";
    if(inet_pton(AF_INET, ipAddr, &servaddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n", ipAddr);
        return ;
    }

    if(connect(mSocketFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        close(mSocketFd);
        printf("connect error: %s(errno: %d)\n", strerror(errno),errno);
        return ;
    }
#else
    int len;
    struct sockaddr_un client;
    memset(&client, 0, sizeof(client));
    client.sun_family = AF_UNIX;
    sprintf(client.sun_path, "%s/%05d.socket", AF_UNIX_IPC_CPATH, getpid());
	unlink(client.sun_path);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(client.sun_path);
	if (bind(mSocketFd, (struct sockaddr *)&client, len) < 0){
        printf("bind error: %s(errno: %d)\n", strerror(errno),errno);
        return ;
	}

	len = offsetof(struct sockaddr_un, sun_path) + strlen((char*)AF_UNIX_IPC_SERV);
	if (connect(mSocketFd, (struct sockaddr *)&server, len) < 0){
        printf("connect error: %s(errno: %d)\n", strerror(errno),errno);
        return ;
	}
#endif

    registerClient();

	// 创建客户端读线程
	IPCClient_para_t *pIPCPara = (IPCClient_para_t *)malloc(sizeof(IPCClient_para_t));	 
	pIPCPara->pSelf = this;
	if(0 != CreateNormalThread(readDataFromServerThread, pIPCPara, &mReadTid)){
		free(pIPCPara);
        return ;
	}
	if(0 != CreateNormalThread(writeDataToServerThread, pIPCPara, &mWriteTid)){
		free(pIPCPara);
        return ;
	}

    mbObjIsInited = true;
}

void IPCClient::unInit()
{
    if(false == mbObjIsInited)
        return;

    mbObjIsInited = false;

    close(mSocketFd);
}

int32_t IPCClient::sendDataToClient(int32_t tagId, int32_t type, void *data, int32_t dataLen)
{
    if(!isInited())
        return -1;

    if((NULL == data) || (0 == dataLen))
        return -2;

    IPC_MSG_t msg = {0};
    memcpy(msg.msgHeader, "EAI-BOX", sizeof(msg.msgHeader));
    msg.srcClientId = mClientId;
    msg.dstClientId = tagId;
    msg.msgType  = type + CLIENT_NUM;   //注意要越过内部使用的协议
    msg.msgLen  = dataLen;
    msg.payload = malloc(dataLen);
    if(msg.payload){
        memcpy(msg.payload, data, dataLen);
        // 一个进程多个线程调用同一个sendDataToClient的情况，考虑一下是否需要对队列加锁
        mMsgQueue.push(msg);
    }
    
    return 0;
}

int32_t IPCClient::registerClient()
{
    int32_t data = 0;
    IPC_MSG_t msg = {0};
    memcpy(msg.msgHeader, "EAI-BOX", sizeof(msg.msgHeader));
    msg.srcClientId = mClientId;
    msg.dstClientId = mClientId;
    msg.msgType  = CLIENT_REGISTER;
    msg.msgLen  = sizeof(data);
    msg.payload = malloc(sizeof(data));
    if(msg.payload){
        memcpy(msg.payload, &data, sizeof(data));
        // 一个进程多个线程调用同一个sendDataToClient的情况，考虑一下是否需要对队列加锁
        mMsgQueue.push(msg);
    }

    return 0;
}

int32_t IPCClient::clientSendHeartbeat()
{
    int32_t data = 0;
    IPC_MSG_t msg = {0};
    memcpy(msg.msgHeader, "EAI-BOX", sizeof(msg.msgHeader));
    msg.srcClientId = mClientId;
    msg.dstClientId = mClientId;
    msg.msgType  = CLIENT_HEARTBEAT;
    msg.msgLen  = sizeof(data);
    msg.payload = malloc(sizeof(data));
    if(msg.payload){
        memcpy(msg.payload, &data, sizeof(data));
        // 一个进程多个线程调用同一个sendDataToClient的情况，考虑一下是否需要对队列加锁
        mMsgQueue.push(msg);
    }

    return 0;
}

int32_t IPC_client_create()
{
    IPCClient::createIPCClient();
    return 0;
}
int32_t IPC_client_init(int32_t srvPort, int32_t cliId)
{
    int32_t ret = -1;
    if(!IPCClient::instance()->isInited()){
        IPCClient::instance()->init(srvPort, cliId);
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_unInit()
{
    int32_t ret = -1;
    if(IPCClient::instance()->isInited()){
        IPCClient::instance()->unInit();
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_set_callback(void *pObj, IPC_Client_CB func)
{
    int32_t ret = -1;
    if(IPCClient::instance()->isInited()){
        IPCClient::instance()->setClientCB(pObj, func);
        ret = 0;
    }
    return ret;
}

int32_t IPC_client_sendData(int32_t tagId, int32_t type, void *data, int32_t dataLen)
{
    int32_t ret = -1;
    if(IPCClient::instance()->isInited()){
        ret = IPCClient::instance()->sendDataToClient(tagId, type, data, dataLen);
    }
    return ret;

}


