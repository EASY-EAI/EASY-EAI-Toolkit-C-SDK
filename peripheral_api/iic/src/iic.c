#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define I2C_SLAVE   0x0703
#define I2C_TENBIT  0x0704

/************************************************************************************
功能：
    打开iic设备文件
参数：
    path:设备文件路径
返回值：
    iic设备fd
************************************************************************************/
int32_t iic_init(const char *path)
{
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open i2c\n");
		return false;
    }
    return fd;
}

/************************************************************************************
功能：
    关闭iic设备文件
参数：
    fd: iic设备fd
返回值：
    无
************************************************************************************/
void iic_release(int32_t fd)
{
    close(fd);
}

/************************************************************************************
功能：设置从机地址长度
参数：
    fd：(iic设备fd)  
    len：从机地址长度
返回值：
    成功: 0
    失败: -1
************************************************************************************/
int32_t iic_set_addr_len(int32_t fd, int32_t addrbits)
{
    int32_t par = 0;
    if(7 == addrbits){
        par = 0;
    }else if(10 == addrbits){
        par = 1;
    }else{
        return -1;
    }
    
    int ret =ioctl(fd, I2C_TENBIT, par);
    if(ret < 0){
        perror("set addr_len");
		return -1;
    }
    return 0;
}

/************************************************************************************
功能：
    设置从机地址
参数：
    fd：(iic设备fd)  
    addr：从机地址
返回值：
    成功: 0
    失败: -1
************************************************************************************/
int32_t iic_set_addr(int32_t fd, int32_t addr)
{
    int ret = ioctl(fd, I2C_SLAVE, (addr));    /* 设置从机地址        */
    if (ret < 0) {
        printf("setenv address failed ret: %x \n", ret);
		return -1;
    }
    return 0;
}

/************************************************************************************
功能：
    从指定从机读取iic数据
参数：
    mfd(iic设备fd)
    addr(从机地址)
   *pBuf(接收数据缓存区的地址)
    read_len(缓冲区的最大长度)
返回值：
    读取的数据长度
************************************************************************************/
int32_t iic_read(int32_t fd, unsigned short addr, uint8_t *pBuf, int32_t read_len)
{
//    memset(pBuf,0, sizeof(pBuf));
    
    struct i2c_msg	msg;
	msg.flags = 1;
	msg.addr  = addr;
	msg.len   = read_len;
	msg.buf   = pBuf;
	
    struct	i2c_rdwr_ioctl_data	data;
	data.msgs  = &msg;
	data.nmsgs = 1;

    int len = ioctl(fd, I2C_RDWR, &data);
	if(len < 0){
        perror("read is faild");
		return len;
	}
    return len;
}

/************************************************************************************
功能：
    向指定从机写入iic数据
参数：
    mfd(iic设备fd)
    addr(从机地址)
   *pBuf(待写入数据的地址)
    write_len(待写入数据的长度)
返回值：
    写入的数据长度
************************************************************************************/
int32_t iic_write(int32_t fd, unsigned short addr, uint8_t *pBuf, int32_t write_len)
{
    struct i2c_msg msg;
	msg.flags = 0;
	msg.addr  = addr;
	msg.len   = write_len;
	msg.buf   = pBuf;
	
	struct i2c_rdwr_ioctl_data data;
	data.msgs  = &msg;
	data.nmsgs = 1;	
	
    int len = ioctl(fd, I2C_RDWR, &data);
	if(len < 0){
        perror("write is faild");
		return len;
	}
    return len;
} 

