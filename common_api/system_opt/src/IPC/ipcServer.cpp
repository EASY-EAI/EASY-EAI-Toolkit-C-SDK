#include "ipcServer.h"

typedef struct{
	IPCServer *pSelf;
}IPCServer_para_t;

static const char *errorReason(eChnErr errnum)
{
    switch (errnum)
    {
        case eNoErr:      return "unknow";
        case eNoChn:      return "nothing channel in channel manager";
        case eFdNotFind:  return "can not find this fd in all channel";
        case eChnIsEmpty: return "this channel is empty";
    }

    return "unknow";
}

/* ========================================= Thread ============================================ */
void * monitorClientsThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;
    PRINT_DEBUG("ipc server monitorClientsThread create succ");
    while(1)
    {
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }
        
        /* 遍历map */
        pthread_mutex_lock(&pSelf->IDmap_lock);
        std::map<int32_t, Client_ptr*>::iterator iter = pSelf->mId_SocketFd.begin();
        while (iter != pSelf->mId_SocketFd.end())
        {
            /*判断是否心跳倒计时为0（设置标志，后续做反应）*/
            if(0 == iter->second->timeout)
            {
                //ID_fd_map中剔除失效fd
                free(iter->second);
                iter->second = nullptr;
                iter = pSelf->mId_SocketFd.erase(iter);
                
                // 销毁该链接的资源
                int disable_fd = iter->second->clientFd;//心跳倒计时结束的fd
                pSelf->destoryClientResource(disable_fd);
                
                PRINT_ERROR("[IPCServer]: Can not received client[%d(cliFd)] Heartbeat for a long time! destory related resources.", disable_fd);
            }else{
                /*心跳倒计时自减*/
                iter->second->timeout--;
                PRINT_TRACE("[IPCServer]: Client[%d(cliId)---(%d(cliFd), %d(timeOut))] countdown...", iter->first, iter->second->clientFd,iter->second->timeout);
                iter++;
            }
        }
        pthread_mutex_unlock(&pSelf->IDmap_lock);
        sleep(1);
    }
    
    PRINT_ERROR("[IPCServer]: --- exit monitorClientsThread !!! ---");
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

#define LISTENEVENT_NUM 10
void *clientAcceptThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;

    // 声明epoll_event结构体的变量, events[20]数组用于缓存被激活的事件信息, ev用于遍历events[20]数组。
    int nfds,i;
    struct epoll_event events[LISTENEVENT_NUM];

    // 声明客户端变量，用于链接新建客户端
#if USING_INET
    socklen_t len;
    struct sockaddr_in cliaddr;
    len = sizeof(cliaddr);
#else
    struct stat statbuf;
    socklen_t size;
    struct sockaddr_un cliaddr;
    memset(&cliaddr, 0, sizeof(cliaddr));
	cliaddr.sun_family = AF_UNIX;
	strcpy(cliaddr.sun_path, AF_UNIX_IPC_SERV);
    size = offsetof(struct sockaddr_un, sun_path) + strlen(cliaddr.sun_path);
#endif
    int32_t cliSocketFd;
    
    PRINT_DEBUG("ipc server clientAcceptThread create succ");
    while(1){
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }
        //等待epoll事件的发生
        nfds = epoll_wait(pSelf->mListenEpollFd, events, LISTENEVENT_NUM, 500);
        /* 说明:
         * int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
         *  【epfd】
         *      epoll的fd
         *  【events】
         *      被激活事件信息缓冲数组
         *  【maxevents】
         *      一次性处理(响应)的最大fd数量
         *  【timeout】
         *      超时时间，单位：毫秒。当timeout为-1时，为永久阻塞。当timeout为0时，为立即返回。
         *  【返回值】
         *      实际被激活的fd总数量(小于等于maxevents)。
         */

        usleep(1000*1000);

#if 0
        // 问题：如果多个客户端同时来connect，这里只会处理一个。其它的都会被漏掉了
        PRINT_DEBUG("[IPCServer]: Waitting Accept Client Number: %d ...", nfds);
        // 处理所有被激活的fd
        for(i = 0; i < nfds; ++i) {
            // 如果被激活的fd是listenFd
            if(events[i].data.fd == pSelf->mListenFd) {
#else
        while(1) {
            if(1) {
#endif
                cliSocketFd = -1;
#if USING_INET
                cliSocketFd = accept(pSelf->mListenFd, (struct sockaddr*)&cliaddr, &len);
#else
                cliSocketFd = accept(pSelf->mListenFd, (struct sockaddr*)&cliaddr, &size);
            	cliaddr.sun_path[strlen(cliaddr.sun_path)] = 0;
            	if (stat(cliaddr.sun_path, &statbuf) < 0){
            		PRINT_ERROR("stat error");
            		//exit(1);
            	}
            	if (S_ISSOCK(statbuf.st_mode) == 0){
            		PRINT_ERROR("S_ISSOCK error");
            		//exit(1);
            	}
                
                PRINT_DEBUG("socketPath(%s)", cliaddr.sun_path);
            	unlink(cliaddr.sun_path);
#endif
                if(cliSocketFd > 0){
                    PRINT_DEBUG("[IPCServer]: new client connect %d(cliFd)", cliSocketFd);
                    // 给改链接分配资源
                    pSelf->createClientResource(cliSocketFd);
                    // 注意：
                    //   1、读写缓存通道到与客户端链接关联。若客户端的链接已断开(心跳已停止)则也要销毁对应的读写缓存通道。
                    PRINT_DEBUG("[IPCServer]: new client connected", cliSocketFd);
                }
            }
            usleep(100*1000);
        }
        
    }

    PRINT_ERROR("[IPCServer]: --- exit clientAcceptThread !!! ---");
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

void *channelDataTransmitThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;

    // 声明用于消息转发的相关参数
    IPC_MSG_t msg = {0};
    bool bIsAllChnEmpty = true;
    int32_t ret;
    int32_t srcClientFd, dstClientFd;
    PRINT_DEBUG("ipc server channelDataTransmitThread create succ");
    while(1){
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }

        bIsAllChnEmpty = true;
        // 轮询读通道，把数据转移到写通道上
        pthread_mutex_lock(&pSelf->mSocketList_lock);
        for(auto iter = pSelf->mSocketList.begin(); iter != pSelf->mSocketList.end(); iter++){
            
            srcClientFd = (*iter);

            // 当前读通道非空
            if(!pSelf->mpReadChnDataMgr->data_channel_is_empty(srcClientFd)){
                
                bIsAllChnEmpty = false;
                
                msg.msgLen = pSelf->mpReadChnDataMgr->get_data_payloadSize(srcClientFd);
                if(msg.msgLen <= 0)
                    continue;
                
                msg.payload = NULL;
                msg.payload = malloc(msg.msgLen);
                if(msg.payload){
                    ret = pSelf->mpReadChnDataMgr->get_data_fromChannel(srcClientFd, &msg);
                    // 成功从读队列里取出一个节点
                    if(0 == ret){
                        PRINT_TRACE("[IPCServer]: ---get data from [Read] channel[%d(fd)] succ---", srcClientFd);
                        pSelf->enableEpollEvent(srcClientFd, EPOLLIN);//打开读监听

                        // 获取目标通道
                        dstClientFd = pSelf->getDstSocketFd(msg.dstClientId);
                        if(dstClientFd > 0){

                            // 等待目标通道，直到通道不满
                            while(pSelf->mpWriteChnDataMgr->data_channel_is_full(dstClientFd))
                            {
                                msleep(50);
                            }
                            
                            // 送入目标通道，并打开写监听
                            // 为了减少memcpy次数，提高运行效率。则入列成功后msg.payload需要在get_data_fromChannel里面free掉。
                            ret = pSelf->mpWriteChnDataMgr->send_data_toChannel(dstClientFd, msg);
                            if(0 == ret){
                                PRINT_TRACE("[IPCServer]: ---trans data to [write] channel[%d(fd)] succ---", dstClientFd);
                                pSelf->enableEpollEvent(dstClientFd, EPOLLOUT);//打开写监听
                            }else{
                                PRINT_ERROR("[IPCServer]: ---send msg node to %d(fd) error, reason[%s]---", dstClientFd, errorReason((eChnErr)ret));
                                free(msg.payload);
                            }
                            
                        // 目标客户端未注册，丢弃该包
                        }else{
                            PRINT_ERROR("[IPCServer]: ---get dstClient %d(Id) error---", msg.dstClientId);
                            free(msg.payload);
                        }
                        
                    // 因为节点需要移交给写队列的，所以要取失败才能free
                    }else{
                        PRINT_TRACE("[IPCServer]: ---get msg node from %d(fd) error, reason[%s]---", srcClientFd, errorReason((eChnErr)ret));
                        free(msg.payload);
                    }
                }

            // 读通道为空
            }else{
                PRINT_TRACE("[IPCServer]: ---[Read] channel[%d(fd)] is empty---", srcClientFd);
                pSelf->enableEpollEvent(srcClientFd, EPOLLIN);//打开读监听
            }
            
        }
        pthread_mutex_unlock(&pSelf->mSocketList_lock);
        
        // 所有通道都为空的情况下，不宜轮询过快，以免CPU耗尽
        if(bIsAllChnEmpty){
            msleep(200);
        }
    }

    PRINT_ERROR("[IPCServer]: --- exit channelDataTransmitThread !!! ---");
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

#define IPCEVENT_NUM 20
#define heartbeat_time 10
void *readDataFromClientThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;
    
    // 声明epoll_event结构体的变量, events[20]数组用于缓存被激活的事件信息, ev用于遍历events[20]数组。
    int nfds,i;
    struct epoll_event events[IPCEVENT_NUM];

    // 声明用于消息转发的相关参数
    IPC_MSG_t msg = {0};
    int32_t ret;
    int32_t srcClientFd, dstClientFd;

    PRINT_DEBUG("ipc server readDataFromClientThread create succ");
    while(1){
        // 对象已销毁
        if(NULL == pIPCPara){ break; }
        // IPCClient 未准备就绪
        if(NULL == pSelf) {
            usleep(10*1000);
    	    continue;
        }
        //等待epoll事件的发生
        nfds = epoll_wait(pSelf->mepollFd, events, IPCEVENT_NUM, 500);
        /* 说明:
         * int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
         *  【epfd】
         *      epoll的fd
         *  【events】
         *      被激活事件信息缓冲数组
         *  【maxevents】
         *      一次性处理(响应)的最大fd数量
         *  【timeout】
         *      超时时间，单位：毫秒。当timeout为-1时，为永久阻塞。当timeout为0时，为立即返回。
         *  【返回值】
         *      实际被激活的fd总数量(小于等于maxevents)。
         */

        //usleep(10*1000);
        
        PRINT_TRACE("[IPCServer]: Active Client Number: %d (include read & write)", nfds);
        // 处理所有被激活的fd
        for(i = 0; i < nfds; ++i) {
            
            // 如果被激活的fd是可读的
            if(events[i].events&EPOLLIN) {
                PRINT_TRACE("[IPCServer]: Client %d(fd) is readable", events[i].data.fd);
                // 若fd非法
                if ( (srcClientFd = events[i].data.fd) < 0)
                    continue;
                
                ret = tcp_recv(srcClientFd, &msg, sizeof(msg));
                // 容错处理:
                if(ret < 0){
                    // 读出错
                    usleep(100*1000);
                    continue;
                }else if(0 == ret){
                    // 对端已close 套接字
                    PRINT_ERROR("[IPCServer]: ---Client[%d(fd)] had closed, I should release related resources---", srcClientFd);
                    // 剔除ID map中失效的子项
                    pSelf->delIdMapItemByFd(srcClientFd);
                    // 销毁该链接的资源
                    pSelf->destoryClientResource(srcClientFd);
                    usleep(100*1000);
                    continue;
                }
                
                msg.payload = NULL;
                msg.payload = malloc(msg.msgLen);
                if(msg.payload){
                    // 取出payload
                    ret = tcp_recv(srcClientFd, msg.payload, msg.msgLen);
                    if(ret != msg.msgLen){
                        PRINT_ERROR("[IPCServer]: ---Read a invalid Packet, From srcClientFd(%d)---", srcClientFd);
                        free(msg.payload);
                        continue;
                    }
                    
                    if(CLIENT_REGISTER == msg.msgType){
                        // 注册客户端Id
                        
                        /* 查找是否为重连接，是则销毁旧链接资源 */
                        int32_t needCloseFd = pSelf->getDstSocketFd(msg.srcClientId);
                        if(0 < needCloseFd){
                            // 剔除ID map中失效的子项
                            pSelf->delIdMapItemById(msg.srcClientId);
                            // 销毁该链接的资源
                            pSelf->destoryClientResource(needCloseFd);
                        }
                        
                        /* 建立连接,绑定ID-FD */
                        pSelf->addIdMapItem(msg.srcClientId, srcClientFd);
                        PRINT_DEBUG("[IPCServer]: ---Register clientId succ, <(ID)%d -- (Fd)%d>---", msg.srcClientId, srcClientFd);
                        
                        free(msg.payload);

                    }else if(CLIENT_HEARTBEAT == msg.msgType){
                        // 客户端给过来的心跳
                        
                        /* 应答标志 */
                        bool bNeedToRespond = false;
                        
                        /* 遍历map，以重置心跳倒计时 */
                        pthread_mutex_lock(&pSelf->IDmap_lock);
                        std::map<int32_t, Client_ptr*>::iterator iter = pSelf->mId_SocketFd.begin();
                        while (iter != pSelf->mId_SocketFd.end()) {
                            //找到发送心跳的client_fd
                            if(srcClientFd == iter->second->clientFd) {
                                iter->second->timeout = heartbeat_time;//恢复默认倒计时
                                bNeedToRespond = true;
                                break;   
                            }
                            iter++;
                        }
                        pthread_mutex_unlock(&pSelf->IDmap_lock);
                        
                        /* 回复 */ 
                        if(bNeedToRespond){
                            // 为了减少memcpy次数，提高运行效率。则入列成功后msg.payload需要在get_data_fromChannel里面free掉。
                            ret = pSelf->mpReadChnDataMgr->send_data_toChannel(srcClientFd, msg);
                            if(0 == ret){
                                PRINT_TRACE("[IPCServer]: --- CLIENT_HEARTBEAT --- Reply HB Packet to itSelf[%d(fd)]", srcClientFd);
                                // 如果读缓存通道已满则关闭当前fd的读监听
                                if(pSelf->mpReadChnDataMgr->data_channel_is_full(srcClientFd)){
                                    PRINT_TRACE("[IPCServer]:  ---[Read] channel[%d(fd)] is full---", srcClientFd);
                                    pSelf->disableEpollEvent(srcClientFd, EPOLLIN);//关闭读监听。
                                }
                            }else{
                                PRINT_ERROR("[IPCServer]: ---Push msg node to ReadChannel-%d(Fd) error, reason[%s]---", srcClientFd, errorReason((eChnErr)ret));
                                free(msg.payload);
                            }
                        }else{
                            PRINT_ERROR("[IPCServer]: ---Can not found clinet: %d(Fd)---", srcClientFd);
                            free(msg.payload);
                        }
                        
                    }else if(CLIENT_QUERY == msg.msgType){
                        // 客户端A过来查询客户端B是否已注册
                        int32_t registedClientId;
                        memcpy(&registedClientId, msg.payload, sizeof(registedClientId));
                        PRINT_DEBUG("[IPCServer]: ---CLIENT_QUERY---  clientA[%d(Id)] query clientB[%d(Id)]", msg.srcClientId, registedClientId);
                        // 获取目标通道
                        int32_t registedClientFd = pSelf->getDstSocketFd(registedClientId);
                        if(registedClientFd <= 0){
                            PRINT_ERROR("[IPCServer]: ---Get dstClient(%d) SocketFd faild---", registedClientId);
                            memset(msg.payload, 0, msg.msgLen);
                        }
                        // 为了减少memcpy次数，提高运行效率。则入列成功后msg.payload需要在get_data_fromChannel里面free掉。
                        ret = pSelf->mpReadChnDataMgr->send_data_toChannel(srcClientFd, msg);
                        if(0 == ret){
                            PRINT_TRACE("[IPCServer]: ---Reply Client Query Packet to itSelf, srcClientFd[%d]---", srcClientFd);
                            // 如果读缓存通道已满则关闭当前fd的读监听
                            if(pSelf->mpReadChnDataMgr->data_channel_is_full(srcClientFd)){
                                PRINT_TRACE("[IPCServer]:  ---[Read] channel[%d(fd)] is full---", srcClientFd);
                                pSelf->disableEpollEvent(srcClientFd, EPOLLIN);//关闭读监听。
                            }
                        }else{
                            PRINT_ERROR("[IPCServer]: ---Push msg node to ReadChannel-%d(Fd) error, reason[%s]---", srcClientFd, errorReason((eChnErr)ret));
                            free(msg.payload);
                        }
                    }else{
                        PRINT_DEBUG("[IPCServer]: ---Push msg node to ReadChannel, [(src)%d >> (dst)%d]---", msg.srcClientId, msg.dstClientId);
                        // 为了减少memcpy次数，提高运行效率。则入列成功后msg.payload需要在get_data_fromChannel里面free掉。
                        ret = pSelf->mpReadChnDataMgr->send_data_toChannel(srcClientFd, msg);
                        if(0 == ret){
                            // 如果读缓存通道已满则关闭当前fd的读监听
                            if(pSelf->mpReadChnDataMgr->data_channel_is_full(srcClientFd)){
                                PRINT_TRACE("[IPCServer]: ---[Read] channel[%d(fd)] is full---", srcClientFd);
                                pSelf->disableEpollEvent(srcClientFd, EPOLLIN);//关闭读监听。
                            }
                        }else{
                            PRINT_ERROR("[IPCServer]: ---Push msg node to ReadChannel-%d(Fd) error, reason[%s]---", srcClientFd, errorReason((eChnErr)ret));
                            free(msg.payload);
                        }
                    }
                }
                // 此处控制一下频率，以免数据瞬间涌入通道
                //usleep(5*1000);
            
            // 如果被激活的fd是可写的
            }else if(events[i].events&EPOLLOUT) {
                PRINT_TRACE("[IPCServer]: Client %d(fd) is writable", events[i].data.fd);
                // 若fd非法
                if ( (dstClientFd = events[i].data.fd) < 0)
                    continue;
                
                // 如果写缓存通道已空则关闭目标fd的写监听
                if(pSelf->mpWriteChnDataMgr->data_channel_is_empty(dstClientFd)){
                    PRINT_TRACE("[IPCServer]: ---[Write] channel[%d(fd)] is empty---", dstClientFd);
                    pSelf->disableEpollEvent(dstClientFd, EPOLLOUT);//关闭写监听。
                }else{
                    msg.msgLen = pSelf->mpWriteChnDataMgr->get_data_payloadSize(dstClientFd);
                    if(msg.msgLen <= 0)
                        continue;
                        
                    msg.payload = NULL;
                    msg.payload = malloc(msg.msgLen);
                    if(msg.payload){
                        ret = pSelf->mpWriteChnDataMgr->get_data_fromChannel(dstClientFd, &msg);
                        if(0 == ret){
                            if(CLIENT_NUM <= msg.msgType){
                                PRINT_DEBUG("[IPCServer]: ---Send data to dstClient, [(src)%d >> (dst)%d]---", msg.srcClientId, msg.dstClientId);
                            }else{
                                PRINT_TRACE("[IPCServer]: ---Send data to dstClient, [(src)%d >> (dst)%d]---", msg.srcClientId, msg.dstClientId);
                            }
                            pthread_mutex_lock(&pSelf->IDmap_lock);
                            for(auto iter = pSelf->mId_SocketFd.begin(); iter != pSelf->mId_SocketFd.end();iter++){
                                if(dstClientFd == iter->second->clientFd){
                                	tcp_send(dstClientFd, &msg, sizeof(msg));
                                	tcp_send(dstClientFd, msg.payload, msg.msgLen);
                                    break;
                                }
                            }
                            pthread_mutex_unlock(&pSelf->IDmap_lock);
                        }else{
                            PRINT_ERROR("[IPCServer]: ---Pop msg node from WriteChannel -- %d(Fd) error, reason[%s]---", dstClientFd, errorReason((eChnErr)ret));
                        }
                        free(msg.payload);
                    }
                }
            }
        
        }

    }

    PRINT_ERROR("[IPCServer]: --- exit readDataFromClientThread !!! ---");
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

/* =========================================== IPC =============================================*/
IPCServer *IPCServer::m_pSelf = NULL;
IPCServer::IPCServer() :
    mListenFd(-1),
    mListenEpollFd(-1),
    mepollFd(-1),
    mpReadChnDataMgr(NULL),
    mpWriteChnDataMgr(NULL),
    mPort(0),
	mbObjIsInited(false)
{
    // 创建用于存放客户端socket文件的目录
    char cmd[128] = {0};
    sprintf(cmd, "mkdir %s", AF_UNIX_IPC_CPATH);
    exec_cmd_by_system(cmd);
    // 清空该目录内所有文件
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "rm %s/*", AF_UNIX_IPC_CPATH);
    exec_cmd_by_system(cmd);

    
	pthread_mutex_init(&mSocketList_lock, NULL);
	
    // 初始化客户端Fd--Events互斥锁
	pthread_mutex_init(&mFdEvents_lock, NULL);
    
    // 初始化客户端Id map互斥锁
	pthread_mutex_init(&IDmap_lock, NULL);
    
	// 创建读、写通道管理器
    mpReadChnDataMgr = new ChnDataMgr;
    mpWriteChnDataMgr = new ChnDataMgr;
}

IPCServer::~IPCServer()
{
    unInit();
    
    if(mpReadChnDataMgr)
        delete mpReadChnDataMgr;
    if(mpWriteChnDataMgr)
        delete mpWriteChnDataMgr;
    
	pthread_mutex_destroy(&IDmap_lock);
	
	pthread_mutex_destroy(&mFdEvents_lock);
	
	pthread_mutex_destroy(&mSocketList_lock);
}

void IPCServer::createIPCServer()
{
    if(m_pSelf == NULL){
        m_pSelf = new IPCServer();
   }
}

void IPCServer::init( int32_t clientMaxNum, int32_t port)
{
    mPort = port;

	//1. 创建一个未连接TCP套接字
#if USING_INET
	mListenFd = socket(AF_INET, SOCK_STREAM, 0);
#else
	mListenFd = socket(AF_UNIX, SOCK_STREAM, 0);
#endif
    PRINT_DEBUG("ListenFd: %d\n", mListenFd);
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
     
	//2. 设置端口复用，避免进程异常退出而导致的资源占用的问题。
	int opt = 1;
    //setsockopt(mListenFd, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
    setsockopt(mListenFd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    /* 说明:
     * int setsockopt( int socket, int level, int option_name, const void *option_value, size_t option_len);
     *  【socket】：
     *      服务器套接字
     *  【level】
     *      被设置的选项的级别。
     *      SOL_SOCKET：套接字级别
     *      IPPROTO_TCP：TCP协议级别
     *  【option_name】
     *      SO_DEBUG，打开或关闭调试信息。(opt==1打开，opt==0关闭)
     *      SO_REUSEADDR，打开或关闭地址复用功能。(opt==1打开，opt==0关闭)
     *      SO_DONTROUTE，打开或关闭路由查找功能。(opt==1打开，opt==0关闭)
     *      SO_BROADCAST，允许或禁止发送广播数据。(opt==1打开，opt==0关闭)
     *      SO_SNDBUF，设置发送缓冲区的大小。
     *      SO_RCVBUF，设置接收缓冲区的大小。
     *      SO_KEEPALIVE，套接字保活。
     *      ·····
     *  【*option_value】
     *      具体的操作参数
     *  【option_len】
     *      操作参数长度
     */
     
	//3. 准备好服务器的结构体变量，然后把服务器的IP地址，协议，端口号绑定到未连接套接字上
#if USING_INET
	//struct sockaddr_in srvaddr;
	srvaddr.sin_family = AF_INET;  //网际协议
	srvaddr.sin_port = htons(port);  //端口号   
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY); // IP地址(INADDR_ANY表示无论是哪个IP地址，只要端口号是对的，都会去处理)
#else
    struct sockaddr_un srvaddr;
	srvaddr.sun_family = AF_UNIX;  //UNIX
    strcpy(srvaddr.sun_path, AF_UNIX_IPC_SERV);
#endif
	/* 说明:
     * sockaddr_un：
     *   UNIX域套接字，仅可以用在“服务器和客户端在同一台机器”上的情况，通常用于进程间通信
     * sockaddr/sockaddr_in：
     *   这两个结构体是同一个东西，指针可以互相强转，只是sockaddr_in的成员写得更加明确
     */
#if USING_INET
	int ret = bind(mListenFd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
#else
    //删除套接字文件，避免因文件存在导致bind()绑定失败	
	unlink(srvaddr.sun_path);
	int ret = bind(mListenFd, (struct sockaddr *)&srvaddr, offsetof(struct sockaddr_un, sun_path) + strlen(srvaddr.sun_path));
#endif
	if(ret == -1){
        close(mListenFd);
		PRINT_ERROR("bind faild! (errorMsg: %s)(errno: %d)", strerror(errno), errno);
    }
    /* 说明:
     * int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
     *  【sockfd】：
     *      服务器套接字
     *  【*addr】
     *      指导bind工作方式的配置
     *      如果不指定端口配置，直接执行在下一步的listen，系统就会随机给你分配一个端口。这样会导致客户端在connect时，不知道该往
     *      哪一个端口去connect
     *  【addrlen】
     *      *addr的数据长度
     *  【返回值】
     *      0：成功
     *     -1：失败，端口被占用了。可以用"netstat -an"命令来查看端口处于什么状态，从而定位端口因什么原因被占用。
     */
	
	//4. 将未连接套接字转换为监听套接字
	listen(mListenFd, 32);
    /* 说明:
     * int listen(int fd, int backlog)
     *  【fd】：
     *      服务器套接字
     *  【backlog】
     *      两条队列的总节点数量
     *      队列一：未经三次握手的“客户端”
     *      队列一：已经三次握手的“客户端”，能在accept获取客户端套接字
     *      最大值为：SOMAXCONN。可通过cat /proc/sys/net/core/somaxconn命令查看，可通过sysctl -w net.core.somaxconn=1024命令修改
     */
     
    // 把listenFd注册到监听epoll里面
    struct epoll_event ev;
    /* 说明:
     *   struct epoll_event
     *   {
     *     uint32_t events;    Epoll events 
     *     epoll_data_t data;     User data variable 
     *   } __attribute__ ((__packed__));
     *
     *  【events】：
     *      EPOLLIN:  读监听（默认水平触发）
     *      EPOLLOUT: 写监听（默认水平触发）
     *      EPOLLLT:  水平触发：只要处于该状态就会持续触发。
     *      EPOLLET:  边缘触发，只会在状态变化时触发。
     *  【data】
     *      typedef union epoll_data {
     *          void *ptr;
     *          int fd;
     *          uint32_t u32;
     *          uint64_t u64;
     *      } epoll_data_t;
     */
    mListenEpollFd = epoll_create(LISTENEVENT_NUM);
    ev.events = EPOLLIN|EPOLLET;   //当3次握手完成，如果不进行accept操作，那么内核只会通知一次
    ev.data.fd = mListenFd;
    epoll_ctl(mListenEpollFd, EPOLL_CTL_ADD, mListenFd, &ev);

    // 创建读写epoll
    mepollFd = epoll_create(clientMaxNum);

	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)malloc(sizeof(IPCServer_para_t));	 
	pIPCPara->pSelf = this;
    mSocketFd_Events.clear();
    mId_SocketFd.clear();
	// 创建监听"添加客户端"线程
	if(0 != CreateNormalThread(clientAcceptThread, pIPCPara, &mAcceptTid)){
		free(pIPCPara);
        return ;
	}
	// 创建服务端读线程
	if(0 != CreateNormalThread(readDataFromClientThread, pIPCPara, &mReadTid)){
		free(pIPCPara);
        return ;
	}
	// 创建读端-写端转发线程
	if(0 != CreateNormalThread(channelDataTransmitThread, pIPCPara, &mTransmitTid)){
		free(pIPCPara);
        return ;
	}

    // 创建客户端心跳监测线程
	if(0 != CreateNormalThread(monitorClientsThread, pIPCPara, &mMonitorTid)){
		free(pIPCPara);
        return ;
	}
    
    mbObjIsInited = true;
}

void IPCServer::unInit()
{
    if(false ==mbObjIsInited)
        return;

    mbObjIsInited = false;
    
    mPort = 0;

    struct epoll_event ev;
    int socketFd;
    
    pthread_mutex_lock(&mSocketList_lock);
    for(auto iter = mSocketList.begin(); iter != mSocketList.end(); iter++){
        
        socketFd = (*iter);
        
        if(socketFd > 0){
            // 关闭epoll监听
            ev.data.fd = socketFd;
            epoll_ctl(mepollFd, EPOLL_CTL_DEL, socketFd, &ev);
            delSocketFd_Events(socketFd);// 移除待关闭链接epoll工作模式记录
            // 销毁读写通道
            mpReadChnDataMgr->destroy_data_channel(socketFd);
            mpWriteChnDataMgr->destroy_data_channel(socketFd);
            // 关闭链接
            close(socketFd);
        }
    }
    mSocketList.clear();
    pthread_mutex_unlock(&mSocketList_lock);
    
    if(mepollFd > 0){
        close(mepollFd);
        mepollFd = -1;
    }
    if(mListenEpollFd > 0){
        close(mListenEpollFd);
        mListenEpollFd = -1;
    }
    if(mListenFd > 0){
        close(mListenFd);
        mListenFd = -1;
    }
}

bool IPCServer::createClientResource(int32_t socketFd)
{
    struct epoll_event ev;

    // 1.创建读写通道
    mpReadChnDataMgr->create_data_channel(socketFd);// 给客户端创建一条读缓存通道
    mpWriteChnDataMgr->create_data_channel(socketFd);// 给客户端创建一条写缓存通道
    
    // 2.把新链接注册进 读写epoll
    ev.events  = EPOLLIN|EPOLLOUT; //读监听同时写监听，水平触发。
    ev.data.fd = socketFd;
    epoll_ctl(mepollFd, EPOLL_CTL_ADD, socketFd, &ev);
    addSocketFd_Events(socketFd, ev.events);// 把新链接的epoll工作模式记录下来
    
    // 3.把Fd加入向量表
    addSocketFdInList(socketFd);
    
    return true;
}

bool IPCServer::destoryClientResource(int32_t socketFd)
{
    // 从通信转发list中剔除
    delSocketFdFromList(socketFd);

    // 关闭epoll监听
    struct epoll_event ev;
    ev.data.fd = socketFd;
    epoll_ctl(mepollFd, EPOLL_CTL_DEL, socketFd, &ev);// 关闭epoll监听
    delSocketFd_Events(socketFd);// 移除待关闭链接epoll工作模式记录

    // 销毁读写通道
    mpReadChnDataMgr->destroy_data_channel(socketFd);
    mpWriteChnDataMgr->destroy_data_channel(socketFd);
    
    // 关闭链接
    close(socketFd);
    return true;
}

bool IPCServer::enableEpollEvent(int32_t socketFd, uint32_t event)
{
    if(EPOLLIN == event){
        PRINT_TRACE("[IPCServer]: ---open client[%d(Fd)] read listen---", socketFd);
    }else if(EPOLLOUT == event){
        PRINT_TRACE("[IPCServer]: ---open client[%d(Fd)] write listen---", socketFd);
    }
    
    struct epoll_event ev;
    ev.events  = getSocketEvents(socketFd);
    ev.events |= event; 
    ev.data.fd = socketFd;
    epoll_ctl(mepollFd, EPOLL_CTL_MOD, socketFd, &ev);
    setSocketFd_Events(socketFd, ev.events);

    return true;
}

bool IPCServer::disableEpollEvent(int32_t socketFd, uint32_t event)
{
    if(EPOLLIN == event){
        PRINT_TRACE("[IPCServer]: ---close client[%d(Fd)] read listen---", socketFd);
    }else if(EPOLLOUT == event){
        PRINT_TRACE("[IPCServer]: ---close client[%d(Fd)] write listen---", socketFd);
    }
    
    struct epoll_event ev;
    ev.events  = getSocketEvents(socketFd);
    ev.events &= ~event; //关闭读监听。
    ev.data.fd = socketFd;
    epoll_ctl(mepollFd, EPOLL_CTL_MOD, socketFd, &ev);
    setSocketFd_Events(socketFd, ev.events);
    
    return true;
}

int32_t IPCServer::getDstSocketFd(int32_t cliId)
{
    int32_t socketFd = -1;
    pthread_mutex_lock(&IDmap_lock);
    auto iter = mId_SocketFd.find(cliId);
    if(iter != mId_SocketFd.end()) {
        socketFd = iter->second->clientFd;
    }
    pthread_mutex_unlock(&IDmap_lock);
    return socketFd;
}

void IPCServer::addIdMapItem(int32_t cliId, int32_t socketFd)
{
    pthread_mutex_lock(&IDmap_lock);
    Client_ptr *clt_ptr = (Client_ptr *)malloc(sizeof(Client_ptr));
    if(clt_ptr){
        clt_ptr->clientFd = socketFd;
        clt_ptr->timeout = heartbeat_time; //心跳默认时间（秒）
        mId_SocketFd.insert(std::pair<int32_t, Client_ptr*>(cliId, clt_ptr));
    }
    pthread_mutex_unlock(&IDmap_lock);
}

void IPCServer::delIdMapItemById(int32_t cliId)
{
    pthread_mutex_lock(&IDmap_lock);
    auto iter = mId_SocketFd.find(cliId);
    if(iter != mId_SocketFd.end()) {
        free(iter->second);
        iter->second = nullptr;
        mId_SocketFd.erase(iter);
    }
    pthread_mutex_unlock(&IDmap_lock);
}

void IPCServer::delIdMapItemByFd(int32_t socketFd)
{
    pthread_mutex_lock(&IDmap_lock);
    for(auto iter = mId_SocketFd.begin(); iter != mId_SocketFd.end();){
        if(socketFd == iter->second->clientFd){
            free(iter->second);
            iter->second = nullptr;
            iter = mId_SocketFd.erase(iter);
        }else{
            iter++;
        }
    }
    pthread_mutex_unlock(&IDmap_lock);
}

bool IPCServer::addSocketFdInList(int32_t socketFd)
{
    pthread_mutex_lock(&mSocketList_lock);
    mSocketList.push_back(socketFd);
    pthread_mutex_unlock(&mSocketList_lock);

    return true;
}

bool IPCServer::delSocketFdFromList(int32_t socketFd)
{
    pthread_mutex_lock(&mSocketList_lock);
    mSocketList.remove(socketFd);
    pthread_mutex_unlock(&mSocketList_lock);

    return true;
}

bool IPCServer::addSocketFd_Events(int32_t socketFd, uint32_t events)
{
    std::map<int32_t, uint32_t>::iterator iter;
    pthread_mutex_lock(&mFdEvents_lock);
    iter = mSocketFd_Events.find(socketFd);
    if(iter != mSocketFd_Events.end()) {
        // 存在则修改
        iter->second = events;
    }else{
        // 不存在则新增
        mSocketFd_Events.insert(std::pair<int32_t, uint32_t>(socketFd, events));
    }
    pthread_mutex_unlock(&mFdEvents_lock);
    return true;
}
bool IPCServer::delSocketFd_Events(int32_t socketFd)
{
    //好像是否实现“删除socketFd-Events对”也无所谓
    pthread_mutex_lock(&mFdEvents_lock);
    pthread_mutex_unlock(&mFdEvents_lock);
    return true;
}

uint32_t IPCServer::getSocketEvents(int32_t socketFd)
{
    uint32_t events = 0;

    std::map<int32_t, uint32_t>::iterator iter;
    pthread_mutex_lock(&mFdEvents_lock);
    iter = mSocketFd_Events.find(socketFd);
    if(iter != mSocketFd_Events.end()) {
        events = iter->second;
    }
    pthread_mutex_unlock(&mFdEvents_lock);

    return events;
}
void IPCServer::setSocketFd_Events(int32_t socketFd, uint32_t events)
{
    std::map<int32_t, uint32_t>::iterator iter;
    pthread_mutex_lock(&mFdEvents_lock);
    iter = mSocketFd_Events.find(socketFd);
    if(iter != mSocketFd_Events.end()) {
        iter->second = events;
    }
    pthread_mutex_unlock(&mFdEvents_lock);
}


int32_t IPC_server_create(int clientMaxNum)
{
    int ret = -1;

    IPCServer::createIPCServer();
    if(!IPCServer::instance()->isInited()){
        IPCServer::instance()->init(clientMaxNum);
        ret = 0;
    }
    PRINT_DEBUG("ipc server create succ\n");
    return ret;
}

