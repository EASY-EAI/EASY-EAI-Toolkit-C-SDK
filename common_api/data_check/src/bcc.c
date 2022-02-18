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

uint8_t block_check_character(uint8_t *hexData, int length)
{
    int sum = 0;
    if((NULL == hexData)||(0 == length))
        return 0;

    for(int i = 0; i < length; i++){
        sum ^= hexData[i];
    }

    return sum;   //hex输出 -- %02x
//    return ((sum/16)*10 + (sum%16));    //十进制输出 -- %d
}