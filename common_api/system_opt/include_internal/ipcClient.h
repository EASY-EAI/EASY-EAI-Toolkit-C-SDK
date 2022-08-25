#ifndef IPCCLIENT_H
#define IPCCLIENT_H

#include "ipcCommon.h"


class IPCClient
{
public:
	IPCClient();
	~IPCClient();

    static IPCClient *instance() { return m_pSelf; }
    static void createIPCClient();

    void init(int32_t cliId, int32_t srvPort = IPCSERVER_PORT);
    void unInit();
    bool isInited(){return mbObjIsInited;}

    void setClientCB(void *pObj, IPC_Client_CB func){mpCliParentObj = pObj;mpCliCBFunc = func;}
    int32_t queryRegisteredClient(int32_t dstClientId);
    bool targetClientIsRegistered();
    int32_t sendDataToClient(int32_t tagId, int32_t type, void *data, int32_t dataLen);

    int32_t updataQueryResult(IPC_MSG_t *pMsg);
        
    pthread_t mReadTid,mWriteTid;
    int32_t mSocketFd; //客户端文件描述符
    void *mpCliParentObj; //客户端所在的父对象
    IPC_Client_CB mpCliCBFunc; //客户端回调接口
    
    //发送队列 MsgQueue;
    std::queue<IPC_MSG_t> mMsgQueue;
private:
    int32_t registerClient();
    int32_t clientSendHeartbeat();
    
    static IPCClient *m_pSelf;

    int32_t mQueryClientId;
    bool mbQueryClientIsRegistered;
    
    int32_t mClientId;
    bool mbObjIsInited;
};


#endif //IPCCLIENT_H
