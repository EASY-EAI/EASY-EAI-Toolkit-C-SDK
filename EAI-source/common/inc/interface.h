#ifndef INTERFACE_H
#define INTERFACE_H

#include "system.h"

#define TAKE_MIN(x, y)  (x>=y? y : x)
typedef void*(*ThreadEntry)(void *p);

//字符串处理
LMO_U32  hexToi(const char *str);
LMO_VOID val_to_str(void *dst_pStr, int src_Val, int bit_num);
//可用 atoi(代替) //LMO_S32 ConvertDecStringToValue(const char *DecStr, int *value);
//可用 atol(代替) //LMO_S32 ConvertDecStringToValue(const char *DecStr, unsigned long *value);
LMO_U64  read_val_from_str(char *buff, const char *strTitle);

//系统时间处理
LMO_VOID mSleep(int ms);
LMO_S32  system_timestamp();
LMO_VOID get_system_DateTime(unsigned long *curDate, unsigned long *curTime);
LMO_VOID set_system_DateTime(int year, int mon, int day, int hour, int min, int second);
LMO_S32  cala_DayNum(int year, int month);
LMO_U8   cala_WeekDay(int y,int m, int d);

//网络参数操作
LMO_S32  get_local_ip(char *interface, char *ip, int ip_len);

//线程操作
LMO_S32  createUIThread(ThreadEntry entry, void *para);
LMO_S32  createThread(ThreadEntry entry, void* para, pthread_t *pid, int priority);
LMO_S32  createNormalThread(ThreadEntry entry, void *para, pthread_t *pid);

//shell操作
LMO_VOID exec_cmd(const char *cmd, char *result);
LMO_S32  SYSTEM(const char * cmdstring);

#endif // INTERFACE_H

