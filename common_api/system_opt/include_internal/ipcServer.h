#ifndef IPCSERVER_H
#define IPCSERVER_H

#include "ipcCommon.h"




class IPCServer
{
public:
	IPCServer();
	~IPCServer();
    static IPCServer *instance() { return m_pSelf; }
    static void createIPCServer();

	void init(int32_t port, int32_t clientMaxNum);
	void unInit();
	bool isInited(){return mbObjIsInited;}
    
    int32_t getDstSocketFd(int32_t id);
    uint32_t getSocketEvents(int32_t socketFd);
    void setSocketFd_Events(int32_t socketFd, uint32_t events);

	pthread_t mAcceptTid;
	pthread_t mReadTid;
    int32_t mListenFd;

    int32_t mListenEpollFd;
    int32_t mepollFd;
    fd_set mSocketFds;  //用作select的Fd集
    std::list<int32_t> mSocketList;
    std::map<int32_t, int32_t> mId_SocketFd; // 客户端Id--Fd对
    std::map<int32_t, uint32_t> mSocketFd_Event; // 客户端Fd--event对
    ChnDataMgr *mpReadChnDataMgr;  //读通道缓存管理器(用于指导读监听状态，若监听一直有响应，会耗尽cpu资源)
    ChnDataMgr *mpWriteChnDataMgr; //写通道缓存管理器(用于指导写监听状态，若监听一直有响应，会耗尽cpu资源)
private:
    static IPCServer *m_pSelf;
    
    int32_t mPort;
	bool mbObjIsInited;
};


#endif //IPCSERVER_H
