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
#define LISTENEVENT_NUM 10
void *clientAcceptThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;

    // 声明epoll_event结构体的变量, events[20]数组用于缓存被激活的事件信息, ev用于遍历events[20]数组。
    int nfds,i;
    struct epoll_event events[LISTENEVENT_NUM], ev;

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
    
    printf("ipc server clientAcceptThread create succ\n");
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

        // 处理所有被激活的fd
        for(i = 0; i < nfds; ++i) {
            // 如果被激活的fd是listenFd
            if(events[i].data.fd == pSelf->mListenFd) {
                cliSocketFd = -1;
#if USING_INET
                cliSocketFd = accept(pSelf->mListenFd, (struct sockaddr*)&cliaddr, &len);
#else
                cliSocketFd = accept(pSelf->mListenFd, (struct sockaddr*)&cliaddr, &size);
            	cliaddr.sun_path[strlen(cliaddr.sun_path)] = 0;
            	if (stat(cliaddr.sun_path, &statbuf) < 0){
            		printf("stat error");
            		//exit(1);
            	}
            	if (S_ISSOCK(statbuf.st_mode) == 0){
            		printf("S_ISSOCK error");
            		//exit(1);
            	}
                printf("########################### path =%s\n", cliaddr.sun_path);
            	unlink(cliaddr.sun_path);
#endif
                if(cliSocketFd > 0){
                    //printf("############################ accept %d ############################\n", cliSocketFd);
                    // 则把新链接注册进 读写epoll
                    ev.events  = EPOLLIN|EPOLLOUT; //读监听同时写监听，水平触发。
                    ev.data.fd = cliSocketFd;
                    epoll_ctl(pSelf->mepollFd, EPOLL_CTL_ADD, cliSocketFd, &ev);
                    // 则把新链接的epoll工作模式记录下来
                    pSelf->mSocketFd_Event.insert(std::pair<int32_t, uint32_t>(cliSocketFd, ev.events));
                    // 则把Fd加入向量表
                    pSelf->mSocketList.push_back(cliSocketFd);
                    // 则给客户端创建一条读缓存通道
                    pSelf->mpReadChnDataMgr->create_data_channel(cliSocketFd);
                    // 则给客户端创建一条写缓存通道
                    pSelf->mpWriteChnDataMgr->create_data_channel(cliSocketFd);
                }
            }
        }
        
    }

    printf("[IPCServer]: --- exit clientAcceptThread !!! ---\n");
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

    struct epoll_event ev;

    // 声明用于消息转发的相关参数
    IPC_MSG_t msg = {0};
    bool bIsAllChnEmpty = true;
    int32_t ret;
    int32_t srcClientFd, dstClientFd;
    printf("ipc server channelDataTransmitThread create succ\n");
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
                        //打开读监听。
                        //printf("[IPCServer]: ---open read listen, (src)%d---\n", srcClientFd);
                        ev.events  = pSelf->getSocketEvents(srcClientFd);
                        ev.events |= EPOLLIN; 
                        ev.data.fd = srcClientFd;
                        epoll_ctl(pSelf->mepollFd, EPOLL_CTL_MOD, srcClientFd, &ev);
                        pSelf->setSocketFd_Events(srcClientFd, ev.events);

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
                                //printf("[IPCServer]: ---open write listen, (src)%d---\n", dstClientFd);
                                ev.events  = pSelf->getSocketEvents(dstClientFd);
                                ev.events |= EPOLLOUT; //打开写监听。
                                ev.data.fd = dstClientFd;
                                epoll_ctl(pSelf->mepollFd, EPOLL_CTL_MOD, dstClientFd, &ev);
                                pSelf->setSocketFd_Events(dstClientFd, ev.events);
                            }else{
                                printf("[IPCServer]: ---Send msg node to %d(fd) error, reason[%s]---\n", dstClientFd, errorReason((eChnErr)ret));
                                free(msg.payload);
                            }
                        }else{
                            printf("[IPCServer]: ---Get dstClient(%d) Fd error---\n", msg.dstClientId);
                            free(msg.payload);
                        }
                        
                    // 因为节点需要移交给写队列的，所以要取失败才能free
                    }else{
                        printf("[IPCServer]: ---Get msg node from %d(fd) error, reason[%s]---\n", srcClientFd, errorReason((eChnErr)ret));
                        free(msg.payload);
                    }
                }

            // 读通道为空
            }else{
                //printf("[IPCServer]: ---open read listen, (src)%d---\n", srcClientFd);
                ev.events  = pSelf->getSocketEvents(srcClientFd);
                ev.events |= EPOLLIN; //打开读监听。
                ev.data.fd = srcClientFd;
                epoll_ctl(pSelf->mepollFd, EPOLL_CTL_MOD, srcClientFd, &ev);
                pSelf->setSocketFd_Events(srcClientFd, ev.events);

            }
            
        }
        
        // 所有通道都为空的情况下，不宜轮询过快，以免CPU耗尽
        if(bIsAllChnEmpty){
            msleep(300);
        }
    }

    printf("[IPCServer]: --- exit channelDataTransmitThread !!! ---\n");
    if(pIPCPara){
        free(pIPCPara);
        pIPCPara = NULL;
    }
	pthread_exit(NULL);
}

#define IPCEVENT_NUM 20
void *readDataFromClientThread(void *para)
{
    // 进程间通信对象
	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)para;
    IPCServer *pSelf = pIPCPara->pSelf;
    
    // 声明epoll_event结构体的变量, events[20]数组用于缓存被激活的事件信息, ev用于遍历events[20]数组。
    int nfds,i;
    struct epoll_event ev,events[IPCEVENT_NUM];

    // 声明用于消息转发的相关参数
    IPC_MSG_t msg = {0};
    int32_t ret;
    int32_t srcClientFd, dstClientFd;
    printf("ipc server readDataFromClientThread create succ\n");
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

        // 处理所有被激活的fd
        for(i = 0; i < nfds; ++i) {
            
            // 如果被激活的fd是可读的
            if(events[i].events&EPOLLIN) {
                // 若fd非法
                if ( (srcClientFd = events[i].data.fd) < 0)
                    continue;
                
                ret = tcp_recv(srcClientFd, &msg, sizeof(msg));
                /* 容错处理
                 * 
                 */
                
                msg.payload = NULL;
                msg.payload = malloc(msg.msgLen);
                if(msg.payload){
                    // 取出payload
                    ret = tcp_recv(srcClientFd, msg.payload, msg.msgLen);
                    /* 容错处理
                     * 
                     */
                    
                    if(CLIENT_REGISTER == msg.msgType){
                        // 注册客户端Id
                        printf("[IPCServer]: ---Register clientId succ, <(ID)%d -- (Fd)%d>---\n", msg.srcClientId, srcClientFd);
                        pSelf->mId_SocketFd.insert(std::pair<int32_t, int32_t>(msg.srcClientId, srcClientFd));
                        free(msg.payload);
                        
                    }else if(CLIENT_HEARTBEAT == msg.msgType){
                        // 客户端给过来的心跳
                        free(msg.payload);
                    
                    }else{
                        // 注意要越过内部使用的协议
                        msg.msgType -= CLIENT_NUM;
                        //printf("[IPCServer]: ---Push msg node to ReadChannel, [(src)%d >> (dst)%d]---\n", msg.srcClientId, msg.dstClientId);
                        // 为了减少memcpy次数，提高运行效率。则入列成功后msg.payload需要在get_data_fromChannel里面free掉。
                        ret = pSelf->mpReadChnDataMgr->send_data_toChannel(srcClientFd, msg);
                        if(0 == ret){
                            // 如果读缓存通道已满则关闭当前fd的读监听
                            if(pSelf->mpReadChnDataMgr->data_channel_is_full(srcClientFd)){
                                //printf("[IPCServer]: ---Close read listen, (src)%d---\n", srcClientFd);
                                ev.events  = pSelf->getSocketEvents(srcClientFd);
                                ev.events &= ~EPOLLIN; //关闭读监听。
                                ev.data.fd = srcClientFd;
                                epoll_ctl(pSelf->mepollFd, EPOLL_CTL_MOD, srcClientFd, &ev);
                                pSelf->setSocketFd_Events(srcClientFd, ev.events);
                            }
                        }else{
                            printf("[IPCServer]: ---Push msg node to ReadChannel-%d(Fd) error, reason[%s]---\n", srcClientFd, errorReason((eChnErr)ret));
                            free(msg.payload);
                        }
                    }
                }
            
            // 如果被激活的fd是可写的
            }else if(events[i].events&EPOLLOUT) {
                // 若fd非法
                if ( (dstClientFd = events[i].data.fd) < 0)
                    continue;
                
                // 如果写缓存通道已空则关闭目标fd的写监听
                if(pSelf->mpWriteChnDataMgr->data_channel_is_empty(dstClientFd)){
                    //printf("[IPCServer]: ---Close write listen, (src)%d---\n", dstClientFd);
                    ev.events  = pSelf->getSocketEvents(dstClientFd);
                    ev.events &= ~EPOLLOUT; //关闭写监听。
                    ev.data.fd = dstClientFd;
                    epoll_ctl(pSelf->mepollFd, EPOLL_CTL_MOD, dstClientFd, &ev);
                    pSelf->setSocketFd_Events(dstClientFd, ev.events);
                }else{
                    msg.msgLen = pSelf->mpWriteChnDataMgr->get_data_payloadSize(dstClientFd);
                    if(msg.msgLen <= 0)
                        continue;
                        
                    msg.payload = NULL;
                    msg.payload = malloc(msg.msgLen);
                    if(msg.payload){
                        ret = pSelf->mpWriteChnDataMgr->get_data_fromChannel(dstClientFd, &msg);
                        if(0 == ret){
                            tcp_send(dstClientFd, &msg, sizeof(msg));
                            tcp_send(dstClientFd, msg.payload, msg.msgLen);
                        }else{
                            printf("[IPCServer]: ---Pop msg node from WriteChannel-%d(Fd) error, reason[%s]---\n", dstClientFd, errorReason((eChnErr)ret));
                        }
                        free(msg.payload);
                    }
                }
            }
        
        }

    }

    printf("[IPCServer]: --- exit readDataFromClientThread !!! ---\n");
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
}

void IPCServer::createIPCServer()
{
    if(m_pSelf == NULL){
        m_pSelf = new IPCServer();
   }
}

void IPCServer::init(int32_t port, int32_t clientMaxNum)
{
    mPort = port;

	//1. 创建一个未连接TCP套接字
#if USING_INET
	mListenFd = socket(AF_INET, SOCK_STREAM, 0);
#else
	mListenFd = socket(AF_UNIX, SOCK_STREAM, 0);
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
		printf("bind faild! (errorMsg: %s)(errno: %d)\n", strerror(errno), errno);
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
    mListenEpollFd = epoll_create(4);
    ev.events = EPOLLIN|EPOLLET;   //当3次握手完成，如果不进行accept操作，那么内核只会通知一次
    ev.data.fd = mListenFd;
    epoll_ctl(mListenEpollFd, EPOLL_CTL_ADD, mListenFd, &ev);

    // 创建读写epoll
    mepollFd = epoll_create(clientMaxNum);

	IPCServer_para_t *pIPCPara = (IPCServer_para_t *)malloc(sizeof(IPCServer_para_t));	 
	pIPCPara->pSelf = this;
    mSocketFd_Event.clear();
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
	if(0 != CreateNormalThread(channelDataTransmitThread, pIPCPara, &mReadTid)){
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
    
    for(auto iter = mSocketList.begin(); iter != mSocketList.end(); iter++){
        
        socketFd = (*iter);
        
        if(socketFd > 0){
            // 关闭epoll监听
            ev.data.fd = socketFd;
            epoll_ctl(mepollFd, EPOLL_CTL_DEL, socketFd, &ev);
            // 关闭链接
            close(socketFd);
        }
    }
    mSocketList.clear();
    
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

int32_t IPCServer::getDstSocketFd(int32_t id)
{
    int32_t socketFd = -1;

    std::map<int32_t, int32_t>::iterator iter;
    iter = mId_SocketFd.find(id);
    if(iter != mId_SocketFd.end()) {
        socketFd = iter->second;
    }

    return socketFd;
}

uint32_t IPCServer::getSocketEvents(int32_t socketFd)
{
    uint32_t events = 0;

    std::map<int32_t, uint32_t>::iterator iter;
    iter = mSocketFd_Event.find(socketFd);
    if(iter != mSocketFd_Event.end()) {
        events = iter->second;
    }

    return events;
}
void IPCServer::setSocketFd_Events(int32_t socketFd, uint32_t events)
{
    std::map<int32_t, uint32_t>::iterator iter;
    iter = mSocketFd_Event.find(socketFd);
    if(iter != mSocketFd_Event.end()) {
        iter->second = events;
    }
}

int32_t IPC_server_create(int port, int clientMaxNum)
{
    int ret = -1;

    IPCServer::createIPCServer();
    if(!IPCServer::instance()->isInited()){
        IPCServer::instance()->init(port, clientMaxNum);
        ret = 0;
    }
    printf("ipc server create succ\n");
    return ret;
}

