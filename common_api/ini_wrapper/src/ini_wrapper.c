/**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * this interface repackaged by the third-party open source library (libini), libini version: 1.1.10
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

//===========================================system===========================================
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <iconv.h>
#include <fcntl.h>
#include <dirent.h>
#include <dirent.h>
#include <semaphore.h>

#include "libini.h"

#include "ini_wrapper.h"

int32_t ini_read_int(const char * file, const char * pcSection, const char * pcKey, int * pVal)
{
	int res = 0;
    //errno = 0

	//文件是否存在
	if (access(file, F_OK) == -1)
	{
        return -1;
	}

	//打开文件
	ini_fd_t ini_file = ini_open(file, "r", ";");
	if (ini_file == 0)
	{ 
//        PRINT_FAULT("ini_open %s error!,%s\n" , file,strerror(errno));
        printf("ini_open %s error!,%s\n" , file,strerror(errno));
        return -1;
	}

	//读取配置信息
	ini_locateHeading(ini_file, pcSection);
	
	ini_locateKey(ini_file, pcKey);
	
	res = ini_readInt(ini_file, pVal);
	if(-1 == res)
	{
//		PRINT_FAULT("ini_readInt error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
        printf("ini_readInt error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
	}

	//关闭文件
	ini_close(ini_file);
	if (-1 == res)
    {
        return -1;
    }

    return 0;
}


int32_t ini_read_string(const char *file, const char *pcSection, const char * pcKey, char * pcStr, int len)
{
    int res = 0;

	//文件是否存在
	if (access(file, F_OK) == -1)
    {
        return -1;
	}

	//打开文件
	ini_fd_t ini_file = ini_open(file, "r", ";");
	if (ini_file == 0)
	{ 
//        PRINT_FAULT("ini_open error!\n");
        printf("ini_open error!\n");
        return -1;
	}

	//读取配置信息
	ini_locateHeading(ini_file, pcSection);
	
	ini_locateKey(ini_file, pcKey);
	
	res = ini_readString(ini_file, pcStr, len);
	if(-1 == res)
	{
//		PRINT_FAULT("ini_readString error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
        printf("ini_readString error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
	}

	//关闭文件
	ini_close(ini_file);
	if (-1 == res)
    {
        return -1;
    }

    return 0;
}


int32_t ini_write_int(const char *file, const char *pcSection, const char *pcKey, int Val)
{
	int res = 0;
    //errno = 0;

	//打开文件
    ini_fd_t ini_file = ini_open(file, "wb", ";");
	if (ini_file == 0)
	{ 
//		PRINT_FAULT("ini_open error!,%s\n",strerror(errno));
        printf("ini_open error!,%s\n",strerror(errno));
        return -1;
	}

	//写入配置信息
	ini_locateHeading(ini_file, pcSection);
	
	ini_locateKey(ini_file, pcKey);

	res = ini_writeInt(ini_file, Val);
	if(-1 == res)
	{
//		PRINT_FAULT("ini_writeInt error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
        printf("ini_writeInt error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
	}

	//关闭文件
	ini_close(ini_file);
	if (-1 == res)
	{
//		PRINT_FAULT("IniWriteInt error,%s\n",strerror(errno));
        printf("IniWriteInt error,%s\n",strerror(errno));
        return -1;
    }

    return 0;
}


int32_t ini_write_string(const char *file, const char *pcSection, const char *pcKey, const char *pcStr)
{
	//打开文件
    int res = 0;

    ini_fd_t ini_file = ini_open(file, "w", ";");
	if (ini_file == 0)
	{ 
//		PRINT_FAULT("ini_open error!\n");
        printf("ini_open error!\n");
        return -1;
	}

	//写入配置信息
	ini_locateHeading(ini_file, pcSection);
	
	ini_locateKey(ini_file, pcKey);
	
	res = ini_writeString(ini_file, pcStr);
	if(-1 == res)
	{
//		PRINT_FAULT("ini_writeString error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
        printf("ini_writeString error!,%s,[%s]->%s\n",strerror(errno),pcSection,pcKey);
	}

	//关闭文件
	ini_close(ini_file);
	if (-1 == res)
	{
        return -1;
    }
    return 0;
}

