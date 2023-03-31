#include <iostream>
#include <sstream>
#include <memory>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "iic.h"

#define    I2C_SLAVE       0x0703
#define    I2C_TENBIT      0x0704
#define    DATA_LEN        8

using namespace std;

/*
功能：打开iic设备文件
参数：path（设备文件路径）
返回值：iic设备fd
*/
int32_t iic_init(const char  *path)
{
    int fd;
    fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open i2c\n");
		return false;
    }
    return fd;
}


/*
功能：关闭iic设备文件
参数：mfd（iic设备fd）
返回值：成功true，失败-1
*/
bool iic_release(int32_t mfd)
{
    int ret = close(mfd);
    if (ret < 0) {
        perror("close i2c\n");
		return false;
    }
    return true;
}

/*
功能：设置从机地址长度
参数：mfd(iic设备fd)  len：从机地址长度
返回值：成功（true） 失败（-1）
*/
bool iic_set_addr_len(int32_t mfd,int32_t len = 7)
{
    int32_t par = 0;
    if(7 == len){
        par = 0;
    }else if(10 == len){
        par = 1;
    }
    int ret =ioctl(mfd, I2C_TENBIT, par);
    if(ret < 0){
        perror("set addr_len");
		return false;
    }
    return true;
}

/*
功能：设置从机地址
参数：mfd(iic设备fd)  addr：从机地址
返回值：成功（true） 失败（-1）
*/
bool iic_set_addr(int32_t mfd,int32_t addr)
{
    int ret = ioctl(mfd, I2C_SLAVE, (addr));    /* 设置从机地址        */
    if (ret < 0) {
        printf("setenv address failed ret: %x \n", ret);
		return false;
    }
    return true;
}


/*
功能：读取iic数据
参数：mfd(iic设备fd)  addr(从机地址)  *buf（接收数据缓存区的地址） read_len(缓冲区长度)
返回值：读取的数据长度
*/
bool iic_read(int32_t mfd ,unsigned short addr, uint8_t* buf , int32_t read_len)
{
    struct i2c_msg	msg;
    struct	i2c_rdwr_ioctl_data	data;
    memset(buf,0, sizeof(buf));
	msg.addr = addr;
	msg.flags =1 ;
	msg.len = read_len;
	msg.buf = buf;
	
	data.nmsgs = 1;
	data.msgs = &msg;
	if(ioctl(mfd, I2C_RDWR, &data) < 0){
        perror("read is faild");
		return false;
	}
    return true;
}

/*
功能：写入iic数据
参数：mfd(iic设备fd) addr(从机地址)  *buf（数据的地址） write_len(数据长度)
返回值：写入的数据长度
*/
bool iic_write(int32_t mfd,unsigned short addr,uint8_t *buf,int32_t write_len)
{
    struct i2c_msg	msg;
	struct	i2c_rdwr_ioctl_data	data;
	
	msg.addr = addr;
	msg.flags = 0;
	msg.len = write_len;
	msg.buf = buf;
	
	data.nmsgs = 1;
	data.msgs = &msg;	
	
	if(ioctl(mfd, I2C_RDWR, &data) < 0){
        perror("write is faild");
		return false;
	}
    return true;
} 

