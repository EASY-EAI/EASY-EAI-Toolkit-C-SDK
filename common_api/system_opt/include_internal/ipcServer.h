#ifndef IPCSERVER_H
#define IPCSERVER_H

#include "ipcCommon.h"


typedef struct {
    /* data */
    int32_t clientFd;
    int32_t timeout;
}Client_ptr;

class IPCServer
{
public:
	IPCServer();
	~IPCServer();
    static IPCServer *instance() { return m_pSelf; }
    static void createIPCServer();

    void init(int32_t clientMaxNum, int32_t port = IPCSERVER_PORT);
    void unInit();
    bool isInited(){return mbObjIsInited;}

    // [已链接]客户端资源管理
    bool createClientResource(int32_t socketFd);
    bool destoryClientResource(int32_t socketFd);
    bool enableEpollEvent(int32_t socketFd, uint32_t event);
    bool disableEpollEvent(int32_t socketFd, uint32_t event);
    // [已注册]客户端资源管理
    int32_t getDstSocketFd(int32_t cliId);
    void addIdMapItem(int32_t cliId, int32_t socketFd);
    void delIdMapItemById(int32_t cliId);
    void delIdMapItemByFd(int32_t socketFd);

    pthread_t mAcceptTid;
    pthread_t mReadTid;
    pthread_t mTransmitTid;
    pthread_t mMonitorTid;

    int32_t mListenFd;
    int32_t mListenEpollFd;
    int32_t mepollFd;
    
    // [已链接]客户端及其通道资源
    pthread_mutex_t mSocketList_lock;//客户端socketFd链表的线程🔓
    std::list<int32_t> mSocketList;
    ChnDataMgr *mpReadChnDataMgr;  //读通道缓存管理器(用于指导读监听状态，若监听一直有响应，会耗尽cpu资源)
    ChnDataMgr *mpWriteChnDataMgr; //写通道缓存管理器(用于指导写监听状态，若监听一直有响应，会耗尽cpu资源)
    // [已注册]客户端Id--[fd,心跳]管理
    pthread_mutex_t IDmap_lock;//心跳操作ID_map时的线程🔓
    std::map<int32_t, Client_ptr*> mId_SocketFd; // 客户端Id--客户端指针对
    
private:
    // [已链接]客户端管理
    bool addSocketFdInList(int32_t socketFd);
    bool delSocketFdFromList(int32_t socketFd);
    bool addSocketFd_Events(int32_t socketFd, uint32_t events);
    bool delSocketFd_Events(int32_t socketFd);
    uint32_t getSocketEvents(int32_t socketFd);
    void setSocketFd_Events(int32_t socketFd, uint32_t events);
    
    // [已链接]客户端及其通道资源
    pthread_mutex_t mFdEvents_lock;//客户端Fd--Events对的线程🔓
    std::map<int32_t, uint32_t> mSocketFd_Events; // 客户端Fd--events对
    
    static IPCServer *m_pSelf;
    
    int32_t mPort;
    bool mbObjIsInited;
};


#endif //IPCSERVER_H
