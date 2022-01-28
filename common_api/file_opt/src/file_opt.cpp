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

//=========================================== C++ ===========================================
#include <string>

//======================================= file_opt =======================================
#include "file_opt.h"

using namespace std;


/*********************************************************************
Function:
Description:
    创建一个名字为全路径的目录
Example:
    bool mk_succ = make_directory("/userdata/Demo/testDir");
parameter:
    *directory : 目录位置
Return:
   0：失败
   1：成功
********************************************************************/
bool make_directory(const char *directory)
{
    char tmp[256], *StartPtr, *EndPtr;
    unsigned int len;
    bool    MakeFlag = true;
    char DirTemp[256];

    if((directory == NULL) || (*(directory + 0) != '/') || (strlen(directory) >= 256))
    {
        return false;//pointer is null or the directory is not an absolute directory
    }
    if(*(directory + 1) == '\0')
    {
        return true;//mkdir "/",so return directly
    }
    StartPtr = (char *)directory + 1;//skip the root directory '/'
    while((EndPtr = strstr(StartPtr,"/")) != NULL)
    {
        if((EndPtr - StartPtr) == 0)
        {
            return false;//two '/',i.e:"/test//test"
        }
        StartPtr = EndPtr + 1;
        if(*StartPtr == '\0')
        {
            break;
        }
    }

    strcpy(DirTemp, directory);
    if(*(DirTemp + strlen(DirTemp) - 1) == '/')
    {
        *(DirTemp + strlen(DirTemp) - 1) = '\0';//the directory is end with '/',replace it with '\0'
    }

    StartPtr = DirTemp + 1;//skip the root directory '/'
    tmp[0] = '/';
    tmp[1] = '\0';
    while((EndPtr = strstr(StartPtr, "/")) != NULL)
    {
        len = EndPtr - StartPtr;
        if(len > 0)
        {
            strncat(tmp, StartPtr, len);
            if((mkdir(tmp, 0777) < 0) && (errno != EEXIST))//directory is exist,so make failed
            {
                MakeFlag = false;//make failed
                break;
            }
            StartPtr = EndPtr + 1;
            if(*StartPtr == '\0')
            {
                break;
            }
            strcat(tmp,"/");
        }
        else
        {
            MakeFlag = false;
            break;
        }
    }

    if(EndPtr == NULL)
    {
        if((mkdir(DirTemp, 0777) < 0) && (errno != EEXIST))
        {
            MakeFlag = false;
        }
    }

    return MakeFlag;
}

/*********************************************************************
Function:
Description:
    在目录后面加上'/'
Example:
	char dirPath[256] = "/userdata/Demo/testDir";
    bool add_succ = add_bias_for_dirpath(dirPath);
	if(add_succ){
		// (dirPath == "/userdata/Demo/testDir/");
	}else{
		// (dirPath == "/userdata/Demo/testDir");		
	}
parameter:
    *path : 目录字符串
Return:
   0：失败
   1：成功
********************************************************************/
bool add_bias_for_dirpath(char *path)
{
    if(path)
    {
        int DirLen = 0;

        while((DirLen < 250) && (*(path + DirLen) != '\0'))//获得字符串的长度
        {
            DirLen ++;
        }

        if(DirLen && (DirLen != 250))
        {
            if(*(path + DirLen - 1) != '/')//最后一个字符不是/
            {
                *(path + DirLen) = '/';
                *(path + DirLen + 1) = '\0';
            }
            return true;
        }
    }

    return false;
}








