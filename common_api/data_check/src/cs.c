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

 
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint8_t check_sum_check(uint8_t *hexData, uint16_t length)
{
    unsigned long sum=0;
    unsigned short i=0;
    for(i=0; i < length; i++)
    {
        sum = sum + hexData[i];
    }
    return (sum%256);	
}


/*
 * 发送方：对要数据累加，得到一个数据和，对和求反，即得到我们的校验值。然后把要发的数据和这个校验值一起发送给接收方。
 */ 
uint8_t TX_CheckSum(uint8_t *buf, uint8_t len) //buf为数组，len为数组长度
{ 
    uint8_t i, ret = 0;
 
    for(i=0; i<len; i++)
    {
        ret += *(buf++);
    }
     ret = ~ret;
    return ret;
}
/* 
 * 接收方：对接收的数据（包括校验和）进行累加，然后加1，如果得到0，那么说明数据没有出现传输错误。
 * （注意，此处发送方和接收方用于保存累加结果的类型一定要一致，否则加1就无法实现溢出从而无法得到0，校验就会无效）
 */
uint8_t RX_CheckSum(uint8_t *buf, uint8_t len) //buf为数组，len为数组长度
{ 
    uint8_t i, ret = 0;
 
    for(i=0; i<len; i++)
    {
        ret += *(buf++);
    }
     ret = ret;
    return ret+1;
}

