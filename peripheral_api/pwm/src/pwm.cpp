/**
 *
 * Copyright 2022 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: CHM <chenhaiman@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#include <iostream>
#include <sstream>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "pwm.h"


using namespace std;

int pwm_init(const char  *path)
{
    int fd = 0;
    string str;
    str.append(path);
    str.append("/export");
    fd = open(str.c_str(), O_WRONLY);
    if(fd < 0){
		perror("open export error");
		exit(1);
	}
    write(fd, "0", 1);
    close(fd);
    return 0;
}

/*
    chip:通道号
    per:周期
*/
int pwm_set_period(const char* chipid, const char* period)
{
    int fd_per;
    string mchip;
    mchip.append(chipid);
    mchip.append("/pwm0/period");
    fd_per = open(mchip.c_str(), O_WRONLY);
    if(fd_per < 0){
		perror("open per error");
		exit(1);
	}
    write(fd_per,period,strlen(period));
    close(fd_per);
    return 0;
}
/*
    chip:通道号
    dut:占空比
*/
int pwm_set_duty_cycle(const char* chipid, const char* duty_cycle)
{
    int fd_dut;
    string mchip;
    mchip.append(chipid);
    mchip.append("/pwm0/duty_cycle");
    fd_dut = open(mchip.c_str(), O_WRONLY);
    if(fd_dut < 0){
		perror("open dut error");
		exit(1);
	}
    write(fd_dut,duty_cycle,strlen(duty_cycle));
    close(fd_dut);
    return 0;
}
/*
    chip:通道号
    ena:使能
*/
int pwm_set_enable(const char* chipid, const char* enable)
{
    int fd_ena;
    string mchip;
    mchip.append(chipid);
    mchip.append("/pwm0/enable");
    fd_ena = open(mchip.c_str(), O_WRONLY);
    if(fd_ena < 0){
		perror("open ena error");
		exit(1);
	}
    write(fd_ena,enable,strlen(enable));
    close(fd_ena);
    return 0;
}

int pwm_release(const char* path)
{
    int fd = 0;
    string str;
    str.append(path);
    str.append("/unexport");
    fd = open(str.c_str(), O_WRONLY);
    if(fd < 0){
		perror("open unexport error");
		exit(1);
	}
    write(fd, "0", 1);
    close(fd);
    return 0;  
}













