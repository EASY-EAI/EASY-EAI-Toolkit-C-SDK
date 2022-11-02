#include <linux/watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <peripheral-watchdog.h>

int32_t wdt_open(int32_t s)
{
    int opts = WDIOS_ENABLECARD;
    int  fd = open("/dev/watchdog", O_RDWR);                                           /* 读写方式打开设备文件         */
    if (fd < 0) {
        printf("device open failed!\r\n");                               /* 设备文件打开失败             */
        return -1;
    }
    int ret = ioctl(fd, WDIOC_SETOPTIONS, &opts);                        /* 开启看门狗                   */   
    if(ret < 0){
        perror("open wdt");
	    exit(-1);
    }
    ret = ioctl(fd, WDIOC_SETTIMEOUT, &s);                           /* 设置新的溢出时间             */
    if(ret < 0){
        perror("set wdt time");
	    exit(-1);
    }
    ret = close(fd);                                                     /* 关闭设备文件                 */
    if(ret < 0){
        perror("close device");
        exit(-1);
    }
    return 0;
}

int32_t wdt_feeddog()
{
    int  fd = open("/dev/watchdog", O_RDWR);                                           /* 读写方式打开设备文件         */
    if (fd < 0) {
        perror("open device");
        exit(-1);
    }
    int ret = ioctl(fd, WDIOC_KEEPALIVE, 0);                 /* 喂狗操作                     */
    if(ret < 0){
        perror("set wdt time");
	    exit(-1);
    }
    ret = close(fd);                                                     /* 关闭设备文件                 */
    if(ret < 0){
        perror("close device");
        exit(-1);
    }
    return 0;
}

int32_t wdt_close()
{
    int  fd = open("/dev/watchdog", O_RDWR);                                           /* 读写方式打开设备文件         */
    if (fd < 0) {
        perror("open device");
        exit(-1);
    }
    if (write(fd, "V", 1) != 1) {
			printf("write WDT_OK_TO_CLOSE failed!");
    }
    int opts = WDIOS_DISABLECARD;   
    int ret = ioctl(fd, WDIOC_SETOPTIONS, &opts);                        /* 关闭看门狗                   */
    if(ret < 0){
        perror("close wdt");
	    exit(-1);
    }
    ret = close(fd);                                                     /* 关闭设备文件                 */
    if(ret < 0){
        perror("close device");
        exit(-1);
    }
    return 0;
}

