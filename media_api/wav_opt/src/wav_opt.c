/**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "wav_opt.h"

int32_t wav_rOpen(const char *filePath, wav_info *pWavInfo)
{
    // 1: 打开WAV格式文件
    int32_t fd = open(filePath, O_RDONLY);
    if(fd <= 0) {
        perror("open() error");
        return fd;
    }

    // 2:读取wav头并，从中提取描述该wav数据的信息
    int n1 = read(fd, &pWavInfo->head, sizeof(pWavInfo->head));
    int n2 = read(fd, &pWavInfo->format, sizeof(pWavInfo->format));
    int n3 = read(fd, &pWavInfo->data, sizeof(pWavInfo->data));

    if(n1 != sizeof(pWavInfo->head) ||
       n2 != sizeof(pWavInfo->format) ||
       n3 != sizeof(pWavInfo->data))
    {
        perror("get wavHeader info failed");
        close(fd);
        return -1;
    }

    return fd;
}

int32_t wav_wOpen(const char *filePath, wav_info *pWavInfo)
{
    // 1: 打开WAV格式文件
    int fd = open(filePath, O_CREAT|O_WRONLY|O_TRUNC, 0777);
    if(fd <= 0) {
        perror("open() error");
        return fd;
    }
    
    // 2: 写WAV格式的文件头
    write(fd, &pWavInfo->head, sizeof(pWavInfo->head));
    write(fd, &pWavInfo->format, sizeof(pWavInfo->format));
    write(fd, &pWavInfo->data, sizeof(pWavInfo->data));
    
    return fd;
}

int32_t wav_close(int32_t fd)
{
    if(fd <= 0){
        printf("invalid fd");
        return -1;
    }else{
        close(fd);
    }
    
    return 0;
}