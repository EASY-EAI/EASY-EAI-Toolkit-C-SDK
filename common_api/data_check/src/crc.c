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

uint8_t CRC_8(uint8_t *hexData, uint16_t length)
{
	unsigned char i;     
	unsigned char crc=0x00;/* 计算的初始crc值 */     
	while(length--)
	{        
		crc ^= *hexData++;  /* 每次先与需要计算的数据异或,计算完指向下一数据 */          
		for (i=8; i>0; --i)   /* 下面这段计算过程与计算一个字节crc一样 */          
		{             
			if (crc & 0x80)                
				crc = (crc << 1) ^ 0x31;            
			else                
				crc = (crc << 1);        
		}    
	}
	return (crc);
}

uint16_t CRC_16(uint8_t *hexData, uint16_t length)
{
	uint16_t Reg_CRC=0xffff;
	uint8_t Temp_reg=0x00;
	uint8_t i,j;

	for(i=0;i<length;i++)
	{
		Reg_CRC^= *hexData++;
		for(j=0;j<8;j++)
		{
			if(Reg_CRC&0x0001)
				Reg_CRC=Reg_CRC>>1^0xA001;
			else
				Reg_CRC >>=1;
		}
	}
	Temp_reg=Reg_CRC>>8;
	return (Reg_CRC<<8|Temp_reg);
}