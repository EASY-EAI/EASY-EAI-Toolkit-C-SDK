#ifndef IPCCOMMON_H
#define IPCCOMMON_H

//=====================  C++  =====================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <queue>
#include <list>
#include <map>
//=====================   C   =====================
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "print_msg.h"
#include "system_opt.h"

#define USING_INET 0
#define AF_UNIX_IPC_CPATH "/tmp/IPCClient"
#define AF_UNIX_IPC_SERV "/tmp/ipcServer.socket"

enum eChnErr{
    eNoErr      = 0,
    eNoChn      = (eNoErr-1),
    eFdNotFind  = (eNoErr-2),
    eChnIsEmpty = (eNoErr-3)
};


enum ClientAction{
    CLIENT_NULL = 0,
    CLIENT_REGISTER,    //注册
    CLIENT_QUERY,       //查询
    CLIENT_HEARTBEAT,   //心跳
    CLIENT_NUM
};

#define CHN_MAX_DATALEN 20  //通道内最大节点数
typedef struct {
    int32_t cliSocketFd;
    int32_t startIndex;
    int32_t endIndex;
    IPC_MSG_t arrayMsg[CHN_MAX_DATALEN];
}SocketChnData_t;

extern int tcp_send(int socket_fd, const void *buf, int size);
extern int tcp_recv(int socket_fd, void *buf, int size);

class ChnDataMgr
{
public:
    ChnDataMgr();
    ~ChnDataMgr();

    void create_data_channel(int32_t socketFd);
    void destroy_data_channel(int32_t socketFd);

    bool data_channel_is_full(int32_t socketFd);
    int32_t send_data_toChannel(int32_t socketFd, IPC_MSG_t msg);
    bool data_channel_is_empty(int32_t socketFd);
    int32_t get_data_payloadSize(int32_t socketFd);
    int32_t get_data_fromChannel(int32_t socketFd, IPC_MSG_t *pMsg);

private:
    bool dataChannelIsFull(int32_t socketFd);
    bool dataChannelIsEmpty(int32_t socketFd);
    void destroy_all_channel();
    
    pthread_rwlock_t chnMgrLock;
    std::list<SocketChnData_t> mChannelList;
};

#endif //IPCCOMMON_H