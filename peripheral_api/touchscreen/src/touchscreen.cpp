#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>


#include "touchscreen.h"
#include "tslib.h"
TsHandle g_handle = NULL;



//屏幕检测线程回调
void *TsEvent_thread_entry(void *para)
{
    struct tsdev *ts = (struct tsdev*)para;
    struct ts_sample_mt *mt_ptr = NULL;
    int max_slots;
    unsigned int pressure[12] = {0}; //用于保存每一个触摸点上一次的按压力,初始为 0,表示松开
    struct input_absinfo slot;
    int i;
    uint32_t event = 0;
    int down_x = -999, down_y = -999;                    //    按下时的x y坐标
    int f_sec =0, f_usec = 0;                            //    压力为255时的时间
    int z_sec =0, z_usec = 0;                            //    压力位0时的时间
    int down_time_sec=0, down_time_usec =0;                //     按下时间
    int area_det_flag = 0;                                //    区域检测标志位
    bool on_flag = false;
    bool long_down_flag = false;
    /* 获取触摸屏支持的最大触摸点数 */
    if (0 > ioctl(ts_fd(ts), EVIOCGABS(ABS_MT_SLOT), &slot)) {
        perror("ioctl error");
        ts_close(ts);
        exit(EXIT_FAILURE);
    }
    max_slots = slot.maximum + 1 - slot.minimum;
    //printf("max_slots: %d\n", max_slots);
    /* 内存分配 */
    mt_ptr = (struct ts_sample_mt *)calloc(max_slots, sizeof(struct ts_sample_mt));
    while(1)
    {
        if (0 > ts_read_mt(ts, &mt_ptr, max_slots, 1)) {
            perror("ts_read_mt error");
            ts_close(ts);
            free(mt_ptr);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < max_slots; i++) {
                if (255 == mt_ptr[i].pressure){   //    压力值为255时
                        if(on_flag == false){
                            on_flag = true;
                            //printf("***********按下\n");

                            event |= TS_PRESS;
                            g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                            event &= ~TS_PRESS;

                            f_sec = mt_ptr[i].tv.tv_sec;                    //    保存按下的时刻
                            f_usec = mt_ptr[i].tv.tv_usec;
                            //    点击区域误差
                            //    第一次点击防止默认值被纳入计算 
                            if( (mt_ptr[i].x - down_x <= 20 && mt_ptr[i].x - down_x >= -20) && (mt_ptr[i].y - down_y <= 20 && mt_ptr[i].y - down_y >= -20)&& down_x != -999 && down_y != -999)
                            {
                                area_det_flag = 1;
                            }

                            down_x = mt_ptr[i].x;                            //    保存当前坐标
                            down_y = mt_ptr[i].y;   
                        }                     
                }else if(0 == mt_ptr[i].pressure){  //    压力值为0时
                        if(on_flag == true){
                                on_flag = false;
                                //printf("***********松开\n");
                                event |= TS_RELEASE;
                                g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                                event &= ~TS_RELEASE;
                            }
                            //    两次点击时间小于0.3s
                            if((mt_ptr[i].tv.tv_sec == z_sec) && ((mt_ptr[i].tv.tv_usec - z_usec) < 300000)|| ((mt_ptr[i].tv.tv_sec == z_sec+1) && (mt_ptr[i].tv.tv_usec - z_usec < -300000)))
                            {
                                if(area_det_flag)                        //    确认两次点击在误差范围内
                                {
                                    area_det_flag = 0;
                                    //printf("***********双击\n");
                                    event |= TS_DOUBLECLICK;
                                    g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                                    event &= ~TS_DOUBLECLICK;
                                }
                            }  else //    为普通点击
                            {        
                                z_sec = mt_ptr[i].tv.tv_sec;                    //    保存松开时的时刻
                                z_usec = mt_ptr[i].tv.tv_usec;
                                //printf("***********单击\n");
                                event |= TS_CLICK;
                                g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                                event &= ~TS_CLICK;
                            }   
                }
                if(on_flag == true){
                    if((mt_ptr[i].tv.tv_sec - f_sec == 1 && mt_ptr[i].tv.tv_usec - f_usec > 500000) || (mt_ptr[i].tv.tv_sec - f_sec > 1 && mt_ptr[i].tv.tv_sec - f_sec < 3))        //    按下时间超过1.5秒为长按
                    {
                        f_sec = 0;
                        f_usec = 0;
                        long_down_flag = true;
                        //printf("x差值：%d , y差值：%d",mt_ptr[i].x - down_x, mt_ptr[i].y - down_y);
                        //printf("***********长按\n");
                        event |= TS_LONGPRESS;
                        g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                        event &= ~TS_LONGPRESS;
                    }
                    if( long_down_flag == true){
                        if((mt_ptr[i].x - down_x >= 80 || mt_ptr[i].x - down_x <= -80) || (mt_ptr[i].y - down_y >= 80 || mt_ptr[i].y - down_y <= -80) && down_x != -999 && down_y != -999){
                            long_down_flag = false;
                            //printf("***********拖动\n");   
                            event |= TS_DRAG;
                            g_handle(event,mt_ptr[i].x, mt_ptr[i].y);
                            event &= ~TS_DRAG;                                      
                        }   
                    }
            }
        }
        // usleep(20*1000);
    }
    ts_close(ts);
    free(mt_ptr);
    exit(EXIT_SUCCESS);
}

//-----------------------------------------------------------------------------
// Return Value : （返回值）
//                1)成功：0
//                2)失败：-1
// Parameters   : （形参列表）
//                1) dev_name：触摸屏的dev(如：1./dev/input/event1  2.NULL(使用环境变量TSLIB_TSDEVICE))
//                2) nonblock：阻塞方式(0：非阻塞 ，1：阻塞)
// -（函数功能说明）
//  （1）打开触摸屏设备及配置触摸屏 ts_setup ==（ts_open，ts_config）配置时解析/etc/ts.conf文件
//  （2）创建触摸屏单点检测线程    
//-----------------------------------------------------------------------------
int  Init_TsEven(const char* dev_name,int nonblock)
{
    pthread_t ThreadId;
    pthread_attr_t attr;
    struct tsdev *ts = NULL;
    /* 打开并配置触摸屏设备 */
    ts = ts_setup(dev_name, nonblock);
    if (NULL == ts) {
        perror( "ts_setup error");
        exit(-1);
    }

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);//绑定
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);//分离
    if(pthread_create(&ThreadId, &attr, TsEvent_thread_entry, ts) == 0)//创建线程
    {
        pthread_attr_destroy(&attr);
        return 0;
    }
    pthread_attr_destroy(&attr);

    return 0;
}

//回调
int set_even_handle(TsHandle handle)
{
    if(NULL == handle)
    {
        return -1;
    }
    g_handle = handle;
    return 0;
}