#include "ipcCommon.h"

/**
 * send size bytes
 */
int tcp_send(int socket_fd, const void *buf, int size)
{
    int count, total_write = 0;

    while(total_write != size)
    {
        count = write(socket_fd, buf, size - total_write);
        if(count == 0) {
            return total_write;
        }
        if(count == -1) {
            return -1;
        }

        total_write += count;
        buf += count;
    }
    return total_write;
}

/**
 * recv size bytes
 */
int tcp_recv(int socket_fd, void *buf, int size)
{
    int count, total_read = 0;
    
    while(total_read != size)
    {
        count = read(socket_fd, buf, size - total_read);
        if(count == 0) {
            return total_read;
        }
        if(count == -1) {
            return -1;
        }

        total_read += count;
        buf += count;
    }
    return total_read;
}

static void free_mem(void **pMem)
{
    if(*pMem){
        free(*pMem);
        *pMem = NULL;
    }
}

ChnDataMgr::ChnDataMgr()
{
    mChannelList.clear();
}

ChnDataMgr::~ChnDataMgr()
{
    destroy_all_channel();
}


void ChnDataMgr::create_data_channel(int32_t socketFd)
{
    SocketChnData_t pChannel = {0};
    pChannel.cliSocketFd = socketFd;
    pChannel.startIndex = 0;
    pChannel.endIndex = 0;
    mChannelList.push_back(pChannel);
}

/* 迭代器失效问题：
 * (迭代器失效会使程序崩溃，或者出现无法预料的结果)
 * · 一个容器：             std::vector<int>
 * · 容器对应的迭代器：std::vector<int>::iteator
 * 
 * 以[SocketChnData_t]为例：
 * · std::vector<SocketChnData_t> mpChannelList;
 * · std::vector<SocketChnData_t>::iteator iter;
 *
 * 错误写法：
 * iter = mpChannelList.begin();
 * mpChannelList.erase(iter);    (一旦容器执行erase()后，迭代器iter就会失效)
 * 正确写法：
 * iter = mpChannelList.begin();
 * iter = mpChannelList.erase(iter); (容器的erase()，会返回下一个有效的迭代器)
 *      或
 * iter = mpChannelList.begin();
 * mpChannelList.erase(iter++); (先把iter传值到erase里面，然后iter自增，最后执行erase，所以iter在失效前已经自增了)
 */
void ChnDataMgr::destroy_data_channel(int32_t socketFd)
{
    if(mChannelList.empty()){
        return ;
    }

    int i;
    for(auto iter = mChannelList.begin(); iter != mChannelList.end();)
    {
        if(socketFd == (*iter).cliSocketFd){
            /* 1 -- 释放通道 内 节点
             */
            // 有效数据在数组中间
            // [ ][ ][ ][s][*]...[*][*][e][ ][ ]   (s:start, e:end)
            if((*iter).startIndex <= (*iter).endIndex){
                for(i = (*iter).startIndex; i < ((*iter).endIndex + 1); i++){
                    free_mem(&(*iter).arrayMsg[i].payload);
                }
            // 有效数据在数组两端
            // [*][*][*][e][ ]...[ ][ ][s][*][*]   (s:start, e:end)
            }else{
                // 释放前段资源
                for(i = 0; i < ((*iter).endIndex + 1); i++){
                    free_mem(&(*iter).arrayMsg[i].payload);
                }
                // 释放后段资源
                for(i = (*iter).startIndex; i < CHN_MAX_DATALEN; i++){
                    free_mem(&(*iter).arrayMsg[i].payload);
                }
            }
            
            /* 2 -- 删除元素
             */
            iter = mChannelList.erase(iter);
        } else {
            iter++;
        }
    }
    
}

bool ChnDataMgr::data_channel_is_full(int32_t socketFd)
{
    if(mChannelList.empty()){
        return true;
    }
    
    // 利用迭代器查找与传入socketFd相同的通道
    auto iter = mChannelList.begin();
    for(; iter != mChannelList.end();iter++){
        if(socketFd == (*iter).cliSocketFd){
        
            // 有效数据在数组中间
            // [ ][ ][ ][s][*]...[*][*][e][ ][ ]   (s:start, e:end)
            if((*iter).startIndex <= (*iter).endIndex){
                if(((*iter).endIndex - (*iter).startIndex) >= (CHN_MAX_DATALEN -1))
                    return true;
                else
                    return false;
            
            // 有效数据在数组两端
            // [*][*][*][e][ ]...[ ][ ][s][*][*]   (s:start, e:end)
            }else{
                if((*iter).endIndex >= ((*iter).startIndex -1))
                    return true;
                else
                    return false;
            }
        }
    }
    
    // 未创建的socketFd通道
    return true;
}

/* 注意：
 * 如果本接口调用失败，则需要在被调用处把msg的payload free掉。
*/
int32_t ChnDataMgr::send_data_toChannel(int32_t socketFd, IPC_MSG_t msg)
{
    int32_t fd = -1;
    if(mChannelList.empty()){
        // 未创建任何通道
        return -1;
    }

    // 利用迭代器查找与传入socketFd相同的通道
    auto iter = mChannelList.begin();
    for(; iter != mChannelList.end();iter++){
        if(socketFd == (*iter).cliSocketFd){
            fd = socketFd;
            break;
        }
    }

    // 找到对应的通道
    if(-1 != fd){
        if(!data_channel_is_full(fd)){
            memcpy((*iter).arrayMsg[(*iter).endIndex].msgHeader, msg.msgHeader, sizeof(msg.msgHeader));
            (*iter).arrayMsg[(*iter).endIndex].srcClientId = msg.srcClientId;
            (*iter).arrayMsg[(*iter).endIndex].dstClientId = msg.dstClientId;
            (*iter).arrayMsg[(*iter).endIndex].msgType     = msg.msgType;
            (*iter).arrayMsg[(*iter).endIndex].msgLen      = msg.msgLen;
            if(msg.payload){
                (*iter).arrayMsg[(*iter).endIndex].payload = msg.payload;  //这个需要在get_data_fromChannel()时free掉
            }

            (*iter).endIndex++;
            (*iter).endIndex%=CHN_MAX_DATALEN;
        }else{
            // 与socketFd对应的通道已满
            return -3;
        }
    }else{
        // 未创建与socketFd对应的通道
        return -2;
    }

    // 数据插入成功
    return 0;
}

bool ChnDataMgr::data_channel_is_empty(int32_t socketFd)
{
    if(mChannelList.empty()){
        return true;
    }
    //printf("aaaaaaaaaaa\n");
    
    // 利用迭代器查找与传入socketFd相同的通道
    auto iter = mChannelList.begin();
    for(; iter != mChannelList.end();iter++){
        
        // 找到对应的通道
        if(socketFd == (*iter).cliSocketFd){
            //printf("bbbbbbbbbbbbb\n");
            if((*iter).startIndex == (*iter).endIndex)
                return true;
            else
                return false;
        }
    }

    // 未创建的socketFd通道
    return true;

}
int32_t ChnDataMgr::get_data_payloadSize(int32_t socketFd)
{
    int32_t fd = -1;
    if(mChannelList.empty()){
        // 未创建任何通道
        return -1;
    }

    // 利用迭代器查找与传入socketFd相同的通道
    auto iter = mChannelList.begin();
    for(; iter != mChannelList.end();iter++){
        if(socketFd == (*iter).cliSocketFd){
            fd = socketFd;
            break;
        }
    }
    
    // 找到对应的通道
    if(-1 != fd){
        if(!data_channel_is_empty(fd)){            
            return (*iter).arrayMsg[(*iter).startIndex].msgLen;
        }else{
            // 与socketFd对应的通道为空
            return -3;
        }
    }else{
        // 未创建与socketFd对应的通道
        return -2;
    }
}

int32_t ChnDataMgr::get_data_fromChannel(int32_t socketFd, IPC_MSG_t *pMsg)
{
    int32_t fd = -1;
    if(mChannelList.empty()){
        // 未创建任何通道
        return -1;
    }

    // 利用迭代器查找与传入socketFd相同的通道
    auto iter = mChannelList.begin();
    for(; iter != mChannelList.end();iter++){
        if(socketFd == (*iter).cliSocketFd){
            fd = socketFd;
            break;
        }
    }
    
    // 找到对应的通道
    if(-1 != fd){
        if(!data_channel_is_empty(fd)){
            memcpy(pMsg->msgHeader, (*iter).arrayMsg[(*iter).startIndex].msgHeader,  sizeof(pMsg->msgHeader));
            pMsg->srcClientId = (*iter).arrayMsg[(*iter).startIndex].srcClientId;
            pMsg->dstClientId = (*iter).arrayMsg[(*iter).startIndex].dstClientId;
            pMsg->msgType     = (*iter).arrayMsg[(*iter).startIndex].msgType;
            pMsg->msgLen      = (*iter).arrayMsg[(*iter).startIndex].msgLen;
            if(pMsg->payload && (*iter).arrayMsg[(*iter).startIndex].payload){
                memcpy(pMsg->payload, (*iter).arrayMsg[(*iter).startIndex].payload, pMsg->msgLen);
            }
            //free掉send_data_toChannel送入时的payload
            free_mem(&(*iter).arrayMsg[(*iter).startIndex].payload);

            (*iter).startIndex++;
            (*iter).startIndex%=CHN_MAX_DATALEN;
        }else{
            // 与socketFd对应的通道为空
            return -3;
        }
    }else{
        // 未创建与socketFd对应的通道
        return -2;
    }

    // 数据成功取出
    return 0;
}

void ChnDataMgr::destroy_all_channel()
{
    if(mChannelList.empty()){
        return ;
    }
    
    int i;
    for(auto iter = mChannelList.begin(); iter != mChannelList.end();iter++)
    {
        /* 1 -- 释放通道 内 节点
         */
        // 有效数据在数组中间
        // [ ][ ][ ][s][*]...[*][*][e][ ][ ]   (s:start, e:end)
        if((*iter).startIndex <= (*iter).endIndex){
            for(i = (*iter).startIndex; i < ((*iter).endIndex + 1); i++){
                free_mem(&(*iter).arrayMsg[i].payload);
            }
        // 有效数据在数组两端
        // [*][*][*][e][ ]...[ ][ ][s][*][*]   (s:start, e:end)
        }else{
            // 释放前段资源
            for(i = 0; i < ((*iter).endIndex + 1); i++){
                free_mem(&(*iter).arrayMsg[i].payload);
            }
            // 释放后段资源
            for(i = (*iter).startIndex; i < CHN_MAX_DATALEN; i++){
                free_mem(&(*iter).arrayMsg[i].payload);
            }
        }
        
    }

    // 清空整个通道管理数据
#if 0    
    auto iter = mpChannelList.begin();
    while (iter != mpChannelList.end()) {
        iter = mpChannelList.erase(iter);
    }
#elif 0
    mpChannelList.erase(mpChannelList.begin(), mpChannelList.end());
#else
    mChannelList.clear();
#endif
}

