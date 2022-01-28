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


/*********************************************************************
Function:
Description:
    把十六进制 字符串 转成对应的 十进制数; 其实等价于上面的 ascii2HexVal;
Example:
    uint32_t val = hexToi("FFFE");
    val == 65534;
parameter:
    *str : 待转换的字符串
Return:
   十六进制字符串对应的十进制数字
********************************************************************/
uint32_t hexToi(const char *str)
{
    uint32_t tmp = 0;
    if((*str == '0') && (*(str+1) == 'x'))//跳过"0x"
        str+=2;
    while(isxdigit(*str)) {//'2'-'0'->2  ;'a'-'a'+10->10;'A'-'A'+10->10
        if(isdigit(*str)) {         //数字字符
            tmp = tmp*16 + (*str-'0');
        } else if(islower(*str)) {  //小写字母
            tmp = tmp*16+ (*str-'a'+10);
        } else {                    //大写字母
            tmp = tmp*16 + (*str-'A'+10);
        }
        str++;
    }

    return tmp;
}

/*********************************************************************
Function:
Description:
    把一个十进制数(0-15,其他值转为空格)转换成字符,其实等价于sprintf(cHex, "%x", val);
    因此不建议使用此函数;
Example:
    char cHex = val_to_char(12);
    cHex == 'C';
parameter:
    val : 待转换的数值
Return:
   十进制数字对应的十六进制字符
********************************************************************/
char val_to_char(int16_t val)
{
    if (0<=val && val<=9)
        return val + '0';
    else if (10 <= val && val <= 15)
        return val-10 + 'A';
    else
        return ' ';
}
/*********************************************************************
Function:
Description:
    把一个十进制数转换成字符串,其实等价于sprintf(dst_pStr, "%x", src_Val);
    因此不建议使用此函数;
Example:
    val_to_hexStr(cpHexNum, 65534, 10);
    结果：cpHexNum == "000000FFFE";
parameter:
    dst_pStr: 用作返回Hex的字符串
    src_Val : 待转换的数值
    bit_num : [cpHexNum]字符串长度
Return:
********************************************************************/
void val_to_hexStr(void *dst_pStr, int src_Val, int bit_num)
{
    char *pTemp;
    unsigned char *pOpt;

    if(NULL == dst_pStr)
        return ;

    pTemp = (char *)malloc(bit_num);
    if(NULL == pTemp)
        return ;

    memset(pTemp, '0', bit_num);
    pOpt = (unsigned char *)(pTemp + bit_num);

    for(int i = 0; i < bit_num; i++){
        if(0 == src_Val)
            break;

        pOpt--;
        *pOpt = val_to_char(src_Val&0x0000000f);

        src_Val = (src_Val>>4);
    }

    memcpy(dst_pStr, pTemp, bit_num);

    free(pTemp);
}

/*********************************************************************
Function:
Description:
    判断一个字符是否是一个10进制的字符
Example:
parameter:
Return:
********************************************************************/
static int IsDecCharacter(char hex)
{
    if((hex >= '0') && (hex <= '9'))
    {
        return (hex - '0');
    }

    return -1;
}

/*********************************************************************
Function:
Description:
    将一个10进制的字符串，转换成其对应的数值,其实等价于atol(const char *__nptr), atol使用更方便
    因此不建议使用此函数;
Example:
    char *str = "7641";
    unsigned long val = 0;
    ConvertDecStringToValue(str, &val);
    结果：val == 7641;
parameter:
Return:
     0 : 成功
    -1 : 失败
********************************************************************/
int32_t ConvertDecStringToValue(const char *DecStr, unsigned long *value)
{
    if(DecStr && value)
    {
        unsigned int index = 0;
        int temp;

        *value = 0;
        while((temp = IsDecCharacter(DecStr[index])) >= 0)//是10进制的字符
        {
            *value = *value * 10 + temp;
            index ++;
        }

        if(strlen(DecStr) == index)//整个字符串分析完成
        {
            return 0;
        }
    }

    return -1;
}

/*********************************************************************
Function:
Description:
    从字符串中提取title后面的值,以'.'、','、' '或字母结尾
Example:
    char strTime[128] = "current Day, Date:2021-11-4, Time:9:39:55";
    uint64_t value = read_val_from_str(strTime, "Date:");
    结果：valuel == 2021114;
parameter:
    *buff     :完整字符串
    *strTitle :定位标签
Return:
********************************************************************/
uint64_t read_val_from_str(char *buff, const char *strTitle)
{
    char *p = NULL;
    int count_p = 0, count_cValue = 0, buffLen = 0, titleLen = 0;

    unsigned long value = 0;
    char strValue[12] = {0};

    p = buff;
    buffLen = strlen(buff); //算上'\n'的
    titleLen = strlen(strTitle);

    while(count_p <= buffLen){
        if(0 == strncmp(p, strTitle, titleLen)){
            p += titleLen;

            while(('.' != *p) && (',' != *p) && (' ' != *p) &&
                   !(('A' <= *p) && (*p <= 'z')) &&
                   ((count_p + titleLen + count_cValue) < buffLen)){
                p++;
                count_cValue++;
            }
            p -= count_cValue;

            memcpy(strValue, p, count_cValue);
            break;
        }
        p++;
        count_p++;
    }
    ConvertDecStringToValue(strValue, &value);

    return value;
}
