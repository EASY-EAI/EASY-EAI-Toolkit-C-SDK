#include "ipcClient.h"

typedef struct{
	IPCClient *pSelf;	
}IPCClient_para_t;

/* ========================================= Thread ============================================ */

int32_t  heartBeat_count = 0;

void *keepAliveThread(void *para)
{
    // 进程间通信对象
	IPCClient_para_t *pIPCPara = (IPCClient_para_t *)para;
    IPCClient *pSelf = pIPCPara->pSelf;
    
    while (1)
    {
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }    

        
        // 3次未收到心跳应答，重新建立连接
        if(heartBeat_count >= 3){
            heartBeat_count = 0;
            PRINT_ERROR("[IPClient(%d)]:===== need reConnect =====", pSelf->clientId());
            pSelf->disConnet();/*销毁旧链接资源*/
            pSelf->connet();/*重新发送连接*/
        }
        
        // 进程间通信通道 已准备就绪
        if(pSelf->isConnected()) {
            // 发送心跳包
            PRINT_TRACE("[IPClient(%d)]:===== send Heart Beat =====", pSelf->clientId());
            pSelf->clientSendHeartbeat();
        }
        heartBeat_count++;
        
        sleep(3);
    }
    
    PRINT_ERROR("[IPClient(%d)]:===== exit keepAliveThread !!! =====", pSelf->clientId());
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

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
        if(!pSelf->isConnected()) {
            usleep(10*1000);
    	    continue;
        }
        
        ret = 0;
        ret = tcp_recv(pSelf->mSocketFd, &msg, sizeof(msg));
        if(0 == ret){
            usleep(200*1000);
            continue;
        }
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
            if(ret != msg.msgLen){
                PRINT_ERROR("[IPClient(%d)]:===== Read a invalid Packet, From Server =====", pSelf->clientId());
                free(msg.payload);
                continue;
            }
            
            if(msg.msgType >= CLIENT_NUM){
                // 注意要越过内部使用的协议
                msg.msgType -= CLIENT_NUM;
                
                // 把payload写入回调
                PRINT_DEBUG("[IPClient(%d)]:===== Recv Data(Type:[%d] - Size:[%d]) from ProcessId[%d] =====", pSelf->clientId(), msg.msgType, msg.msgLen, msg.srcClientId);
                if((pSelf->mpCliParentObj)&&(pSelf->mpCliCBFunc)){
                    pSelf->mpCliCBFunc(pSelf->mpCliParentObj, &msg);
                }
            }else if(msg.msgType == CLIENT_QUERY){
                PRINT_TRACE("[IPClient(%d)]:=========== Recv query Result ===========", pSelf->clientId());
                pSelf->updataQueryResult(&msg);
            }else if(msg.msgType == CLIENT_HEARTBEAT){
                PRINT_TRACE("[IPClient(%d)]:============ Recv Heart Beat ============", pSelf->clientId());
               heartBeat_count = 0;//收到ACK则清除心跳包发送计数
            }
            
            free(msg.payload);
        }
    }
    
    PRINT_ERROR("[IPClient(%d)]:===== exit readDataFromServerThread !!! =====", pSelf->clientId());
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
        if(!pSelf->isConnected()) {
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

        // 数据写入Socket
        if(CLIENT_REGISTER == msg.msgType){
            PRINT_DEBUG("[IPCClient(%d)]:===== Register to Server =====", pSelf->clientId());
        }else if(CLIENT_NUM <= msg.msgType){
            PRINT_DEBUG("============ ProcessId[%d] Send Data(Type:[%d] - Size:[%d]) To ProcessId[%d] ============", msg.srcClientId, msg.msgType-CLIENT_NUM, msg.msgLen, msg.dstClientId);
        }
        tcp_send(pSelf->mSocketFd, &msg, sizeof(msg));
        tcp_send(pSelf->mSocketFd, msg.payload, msg.msgLen);

        if(msg.payload)
            free(msg.payload);
        
        pSelf->mMsgQueue.pop();

        // 此处不要猛发。否则只要消息队列不空，就会被堵在这条线程内
        usleep(5000);

    }
    
    PRINT_ERROR("[IPClient(%d)]:===== exit writeDataToServerThread !!! =====", pSelf->clientId());
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
    mQueryClientId(0),
    mbQueryClientIsRegistered(false),
    mClientId(0),
    mbIsConnected(false),
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

void IPCClient::init(int32_t cliId, int32_t srvPort)
{
    mClientId = cliId;
    
    PRINT_DEBUG("[IPClient(%d)]: init start ", mClientId);
    connet();

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

#if 1
	if(0 != CreateNormalThread(keepAliveThread, pIPCPara, &mKeepAliveTid)){
		free(pIPCPara);
        return ;
	}
#endif
    
    mbObjIsInited = true;
}

void IPCClient::unInit()
{
    if(false == mbObjIsInited)
        return;

    mbObjIsInited = false;

    disConnet();
}

bool IPCClient::targetClientIsRegistered()
{
    // 此处的延时时间，必须 >= 2倍的服务端读写通道转移线程的休眠时间。
    usleep(500*1000);
    
    bool ret = mbQueryClientIsRegistered;
    
    mbQueryClientIsRegistered = false;

    return ret;
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

bool  IPCClient::connet()
{
    if(isConnected()){
        PRINT_ERROR("[IPCClient(%d)]: is connected ...", mClientId);
        return false;
    }
    
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
        PRINT_ERROR("[IPCClient(%d)]: create socket error: %s(errno: %d)", mClientId, strerror(errno),errno);
        return false;
    }
#else
    if((mSocketFd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0){
        PRINT_ERROR("[IPCClient(%d)]: create socket error: %s(errno: %d)", mClientId, strerror(errno),errno);
        return false;
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
        PRINT_ERROR("[IPCClient(%d)]: inet_pton error for %s", mClientId, ipAddr);
        return false;
    }

    if(connect(mSocketFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        close(mSocketFd);
        PRINT_ERROR("[IPCClient(%d)]: connect error: %s(errno: %d)", mClientId, strerror(errno),errno);
        return false;
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
        PRINT_ERROR("[IPCClient(%d)]: bind error: %s(errno: %d)", mClientId, strerror(errno),errno);
        return false;
    }

    len = offsetof(struct sockaddr_un, sun_path) + strlen((char*)AF_UNIX_IPC_SERV);
    if (connect(mSocketFd, (struct sockaddr *)&server, len) < 0){
        PRINT_ERROR("[IPCClient(%d)]: connect error: %s(errno: %d)", mClientId, strerror(errno),errno);
        return false;
    }
#endif
    PRINT_DEBUG("[IPCClient(%d)]: start to register", mClientId);

    registerClient();
    
    mbIsConnected = true;

    return true;
}

bool  IPCClient::disConnet()
{
    if(!isConnected()){
        PRINT_ERROR("[IPCClient(%d)]: is not connected ...", mClientId);
        return false;
    }

    // 读空缓冲区
    int ret = 0;char pTempMem[1024];
    do{ret = tcp_recv(mSocketFd, pTempMem, sizeof(pTempMem));}while(0 < ret);

    // 关闭链接
    close(mSocketFd);
    mSocketFd = -1;

    // 清空发送队列
    while(!mMsgQueue.empty()) mMsgQueue.pop();

    mbIsConnected = false;
    
    return true;
}

int32_t IPCClient::queryRegisteredClient(int32_t dstClientId)
{
    mQueryClientId = dstClientId;

    IPC_MSG_t msg = {0};
    memcpy(msg.msgHeader, "EAI-BOX", sizeof(msg.msgHeader));
    msg.srcClientId = mClientId;
    msg.dstClientId = mClientId;
    msg.msgType  = CLIENT_QUERY;
    msg.msgLen  = sizeof(mQueryClientId);
    msg.payload = malloc(sizeof(mQueryClientId));
    if(msg.payload){
        memcpy(msg.payload, &mQueryClientId, sizeof(mQueryClientId));
        // 一个进程多个线程调用同一个sendDataToClient的情况，考虑一下是否需要对队列加锁
        mMsgQueue.push(msg);
    }

    return 0;
}

int32_t IPCClient::updataQueryResult(IPC_MSG_t *pMsg)
{
    int32_t queryResult = 0;
    memcpy(&queryResult, pMsg->payload, sizeof(queryResult));
    PRINT_DEBUG("[IPCClient(%d)]: (Query) client(%d) is registered.", mClientId, queryResult);
    
    if((0 != queryResult) && (mQueryClientId == queryResult)){
        mbQueryClientIsRegistered = true;
    }else{
        mbQueryClientIsRegistered = false;
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

int32_t IPC_client_create()
{
    IPCClient::createIPCClient();
    return 0;
}
int32_t IPC_client_init(int32_t cliId)
{
    int32_t ret = -1;
    if(!IPCClient::instance()) { return ret; }
    
    if(!IPCClient::instance()->isInited()){
        IPCClient::instance()->init(cliId);
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_unInit()
{
    int32_t ret = -1;
    if(!IPCClient::instance()) { return ret; }
    
    if(IPCClient::instance()->isInited()){
        IPCClient::instance()->unInit();
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_set_callback(void *pObj, IPC_Client_CB func)
{
    int32_t ret = -1;
    if(!IPCClient::instance()) { return ret; }
    
    if(IPCClient::instance()->isInited()){
        IPCClient::instance()->setClientCB(pObj, func);
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_query_registered_client(int32_t dstCliId)
{
    int32_t ret = -1;
    if(!IPCClient::instance()) { return ret; }
    
    if(IPCClient::instance()->isInited()){
        IPCClient::instance()->queryRegisteredClient(dstCliId);
        ret = 0;
    }
    return ret;
}
int32_t IPC_client_dstClient_is_registered()
{
    int32_t ret = 0;
    if(!IPCClient::instance()) { usleep(500*1000); return ret; }
    
    if(IPCClient::instance()->isInited()){
        ret = IPCClient::instance()->targetClientIsRegistered();
    }
    return ret;
}
int32_t IPC_client_sendData(int32_t tagId, int32_t type, void *data, int32_t dataLen)
{
    int32_t ret = -1;
    if(!IPCClient::instance()) { return ret; }
    
    if(IPCClient::instance()->isInited()){
        ret = IPCClient::instance()->sendDataToClient(tagId, type, data, dataLen);
    }
    return ret;

}


