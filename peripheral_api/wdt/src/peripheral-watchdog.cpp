#include <linux/watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <peripheral-watchdog.h>

int gFd = -1;
int32_t wdt_open(int32_t s)
{
    gFd = open("/dev/watchdog1", O_RDWR);   /* 读写方式打开设备文件 */
    if (gFd < 0) {
        printf("device open failed!\r\n");      /* 设备文件打开失败 */
        return -1;
    }
    int ret = 0;
#if 1
    int opts = WDIOS_ENABLECARD;
    ret = ioctl(gFd, WDIOC_SETOPTIONS, &opts);   /* 开启看门狗 */
    if(ret < 0){
        perror("open wdt");
        return -1;
    }
#endif
    ret = ioctl(gFd, WDIOC_SETTIMEOUT, &s);          /* 设置新的溢出时间 */
    if(ret < 0){
        perror("set wdt time");
        return -1;
    }
    
    return s;
}

int32_t wdt_feeddog()
{
    if(gFd < 0)
        return -1;
    
    int ret = ioctl(gFd, WDIOC_KEEPALIVE, 0); /* 喂狗操作 */
    if(ret < 0){
        perror("feed dog");
        return -1;
    }
    
    return 0;
}

int32_t wdt_close()
{
    if(gFd < 0)
        return 0;
    
    if (write(gFd, "V", 1) != 1) {
		printf("write WDT_OK_TO_CLOSE failed!");
    }
    
    int opts = WDIOS_DISABLECARD;   
    int ret = ioctl(gFd, WDIOC_SETOPTIONS, &opts);   /* 关闭看门狗 */
    if(ret < 0){
        perror("close wdt");
        return -1;
    }
    
    ret = close(gFd);    /* 关闭设备文件 */
    if(ret < 0){
        perror("close device");
        return -1;
    }
    
    return 0;
}

