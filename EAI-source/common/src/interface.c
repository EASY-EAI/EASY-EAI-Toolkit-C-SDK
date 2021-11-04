#include "interface.h"


/*********************************************************************
Function:
Description:
    把单一位十六进制 字符 转成对应的 十进制数;
Example:
    int val = ascii2HexVal_single('A');
    val == 10;
parameter:
    c : 待转换的字符
Return:
   十六进制字符对应的十进制数字
********************************************************************/
static LMO_U8 ascii2HexVal_single(LMO_U8 c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else
        return 0;
}
/*********************************************************************
Function:
Description:
    把十六进制 字符串 转成对应的 十进制数; 其实等价于下面的 hexToi, hexToi使用更方便
    因此不建议使用此函数;
Example:
    int val = ascii2HexVal("FFFE", 4);
    val == 65534;
parameter:
    *hexData : 待转换的字符串
    lenth    : 待转换的字符串长度
Return:
   十六进制字符串对应的十进制数字
********************************************************************/
LMO_S32 ascii2HexVal(unsigned char *hexData, int lenth)
{
    int carry_bit = 0;
    int val = 0;
    for(int i = lenth - 1; i>=0; i--){
        //printf("carry_bit = %d, i = %d , hexData = %c\n",carry_bit,i, hexData[i]);
        val += ascii2HexVal_single(hexData[i])*pow(16,carry_bit);
        carry_bit++;
    }
    return val;
}

/*********************************************************************
Function:
Description:
    把十六进制 字符串 转成对应的 十进制数; 其实等价于上面的 ascii2HexVal;
Example:
    int val = hexToi("FFFE");
    val == 65534;
parameter:
    *str : 待转换的字符串
Return:
   十六进制字符串对应的十进制数字
********************************************************************/
LMO_U32 hexToi(const char *str)
{
    int tmp = 0;
    if((*str == '0') && (*(str+1) == 'x'))//跳过"0x"
        str+=2;
    while(isxdigit(*str)) {//'2'-'0'->2  ;'a'-'a'+10->10;'A'-'A'+10->10
        if(isdigit(*str)) {         //数字字符
            tmp = tmp*16 + (*str-'0');
        } else if(islower(*str)) {  //小写字母
            tmp = tmp*16+ (*str-'a'+10);
        } else {                    //大写字母
            tmp = tmp*16 + (*str-'A'+10);
        }
        str++;
    }

    return tmp;
}

/*********************************************************************
Function:
Description:
    把一个十进制数(0-15,其他值转为空格)转换成字符,其实等价于sprintf(cHex, "%x", val);
    因此不建议使用此函数;
Example:
    char cHex = val_to_char(12);
    cHex == 'C';
parameter:
    val : 待转换的数值
Return:
   十进制数字对应的十六进制字符
********************************************************************/
LMO_S8 val_to_char(LMO_S16 val)
{
    if (0<=val && val<=9)
        return val + '0';
    else if (10 <= val && val <= 15)
        return val-10 + 'A';
    else
        return ' ';
}
/*********************************************************************
Function:
Description:
    把一个十进制数转换成字符串,其实等价于sprintf(dst_pStr, "%x", src_Val);
    因此不建议使用此函数;
Example:
    val_to_str(cpHexNum, 65534, 10);
    结果：cpHexNum == "000000FFFE";
parameter:
    dst_pStr: 用作返回Hex的字符串
    src_Val : 待转换的数值
    bit_num : [cpHexNum]字符串长度
Return:
********************************************************************/
LMO_VOID val_to_str(void *dst_pStr, int src_Val, int bit_num)
{
    char *pTemp;
    unsigned char *pOpt;

    if(NULL == dst_pStr)
        return ;

    pTemp = (char *)malloc(bit_num);
    if(NULL == pTemp)
        return ;

    memset(pTemp, '0', bit_num);
    pOpt = (unsigned char *)(pTemp + bit_num);

    for(int i = 0; i < bit_num; i++){
        if(0 == src_Val)
            break;

        pOpt--;
        *pOpt = val_to_char(src_Val&0x0000000f);

        src_Val = (src_Val>>4);
    }

    memcpy(dst_pStr, pTemp, bit_num);

    free(pTemp);
}



/*********************************************************************
Function:
Description:
    判断一个字符是否是一个10进制的字符
Example:
parameter:
Return:
********************************************************************/
static int IsDecCharacter(char hex)
{
    if((hex >= '0') && (hex <= '9'))
    {
        return (hex - '0');
    }

    return -1;
}

/*********************************************************************
Function:
Description:
    将一个10进制的字符串，转换成其对应的数值,其实等价于atoi(const char *__nptr), atoi使用更方便
    因此不建议使用此函数;
Example:
    char *str = "7641";
    int val = 0;
    ConvertDecStringToValue(str, &val);
    结果：val == 7641;
parameter:
Return:
     0 : 成功
    -1 : 失败
********************************************************************/
LMO_S32 ConvertDecStringToValue(const char *DecStr, int *value)
{
    if(DecStr && value)
    {
        unsigned int index = 0;
        int temp;

        *value = 0;
        while((temp = IsDecCharacter(DecStr[index])) >= 0)/*是10进制的字符*/
        {
            *value = *value * 10 + temp;
            index ++;
        }

        if(strlen(DecStr) == index)/*整个字符串分析完成*/
        {
            return 0;
        }
    }

    return -1;
}

/*********************************************************************
Function:
Description:
    将一个10进制的字符串，转换成其对应的数值,其实等价于atol(const char *__nptr), atol使用更方便
    因此不建议使用此函数;
Example:
    char *str = "7641";
    unsigned long val = 0;
    ConvertDecStringToValue(str, &val);
    结果：val == 7641;
parameter:
Return:
     0 : 成功
    -1 : 失败
********************************************************************/
LMO_S32 ConvertDecStringToValue(const char *DecStr, unsigned long *value)
{
    if(DecStr && value)
    {
        unsigned int index = 0;
        int temp;

        *value = 0;
        while((temp = IsDecCharacter(DecStr[index])) >= 0)//是10进制的字符
        {
            *value = *value * 10 + temp;
            index ++;
        }

        if(strlen(DecStr) == index)//整个字符串分析完成
        {
            return 0;
        }
    }

    return -1;
}

/*********************************************************************
Function:
Description:
    从字符串中提取title后面的值,以'.'、','、' '或字母结尾
Example:
    char strTime[128] = "current Day, Date:2021-11-4, Time:9:39:55";
    LMO_U64 value = read_val_from_str(strTime, "Date:");
    结果：valuel == 2021114;
parameter:
    *buff     :完整字符串
    *strTitle :定位标签
Return:
********************************************************************/
LMO_U64 read_val_from_str(char *buff, const char *strTitle)
{
    char *p = NULL;
    int count_p = 0, count_cValue = 0, buffLen = 0, titleLen = 0;

    unsigned long value = 0;
    char strValue[12] = {0};

    p = buff;
    buffLen = strlen(buff); //算上'\n'的
    titleLen = strlen(strTitle);

    while(count_p <= buffLen){
        if(0 == strncmp(p, strTitle, titleLen)){
            p += titleLen;

            while(('.' != *p) && (',' != *p) && (' ' != *p) &&
                   !(('A' <= *p) && (*p <= 'z')) &&
                   ((count_p + titleLen + count_cValue) < buffLen)){
                p++;
                count_cValue++;
            }
            p -= count_cValue;

            memcpy(strValue, p, count_cValue);
            break;
        }
        p++;
        count_p++;
    }
    ConvertDecStringToValue(strValue, &value);

    return value;
}

/*********************************************************************
Function:
Description:
    延时函数
Example:
    mSleep(1000);
parameter:
    ms: 毫秒
********************************************************************/
LMO_VOID mSleep(int ms)
{
    usleep(ms * 1000);
}

/*********************************************************************
Function:
Description:
    获取系统当前时间戳
Example:
    int timestamp = system_timestamp();
parameter:
Return:
     系统时间戳
********************************************************************/
LMO_S32 system_timestamp()
{
    time_t t;
    t = time(NULL);

    return time(&t);
}

/*********************************************************************
Function:
Description:
    以“日期”，“时间”的格式获取系统时间
Example:
    unsigned long curDate = 0;
    unsigned long curTime = 0;
    get_system_DateTime(&curDate, &curTime);
    结果：curDate == 2021114;
    结果：curTime == 93905;
parameter:
    *curDate :当前日期
    *curTime :当前时间
Return:
********************************************************************/
LMO_VOID get_system_DateTime(unsigned long *curDate, unsigned long *curTime)
{
    time_t timer;//time_t就是long int 类型
    struct tm *tblock;

    timer = time(NULL);
    tblock = localtime(&timer);

    *curDate = 10000 * (tblock->tm_year+1900) + 100 * (tblock->tm_mon + 1) + (tblock->tm_mday);
    *curTime = 10000 * (tblock->tm_hour) + 100 * (tblock->tm_min) + (tblock->tm_sec);
}

/*********************************************************************
Function:
Description:
    以“年月日”，“时分秒”的方式设置系统时间
Example:
    set_system_DateTime(2021,11,4,10,14,16);
parameter:
Return:
********************************************************************/
LMO_VOID set_system_DateTime(int year, int mon, int day, int hour, int min, int second)
{
    struct tm t;
    struct timeval tv;
    __time_t time;

    year -= 1900;
    t.tm_year = year;
    t.tm_mon = mon-1;
    t.tm_mday = day;

    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = second;

    time = mktime(&t);
    if(-1 == time){
        perror("mktime");
        return ;
    }

    tv.tv_sec = time;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}


/*********************************************************************
Function:
Description:
    计算出某一年某个月有多少天
Example:
    int DayNum = CalculateDayNum(2021, 11);
parameter:
Return:
********************************************************************/
LMO_S32 cala_DayNum(int year, int month)
{
    int DayNum;

    if (month == 4 || month == 6 || month == 9 || month == 11){
        DayNum = 30;
    } else if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            DayNum = 29;
        else
            DayNum = 28;
    } else {
        DayNum = 31;
    }

    return DayNum;
}

/*********************************************************************
Function:
Description:
    通过某年某月某日，计算出那天是星期几
Example:
    int weekDay = CalculateWeekDay(2021, 11, 3);
parameter:
Return:
********************************************************************/
LMO_U8 cala_WeekDay(int y, int m, int d)
{
    int iWeek;
    if(m==1||m==2) {
        m+=12;
        y--;
    }
    iWeek=(d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;
#ifdef __TEST_SOFT__
    switch(iWeek)
    {
        case 0: printf("星期一\n"); break;
        case 1: printf("星期二\n"); break;
        case 2: printf("星期三\n"); break;
        case 3: printf("星期四\n"); break;
        case 4: printf("星期五\n"); break;
        case 5: printf("星期六\n"); break;
        case 6: printf("星期日\n"); break;
    }
#endif
    return iWeek;
}

/*********************************************************************
Function:
Description:
    通过网卡获取IP地址
Example:
    int ret = get_local_ip("eth0", ip, MAX_IP_LEN);
parameter:
    *interface :网卡名称
    *ip        :用作返回ip的字符串
    ip_len     : [*ip]字符串长度
Return:
     0 : 成功
    -1 : 失败
********************************************************************/
LMO_S32 get_local_ip(char *interface, char *ip, int ip_len)
{
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    memset(ip, 0, ip_len);
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        //qDebug("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, interface, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        //qDebug("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    sprintf(ip, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}

LMO_S32 createUIThread(ThreadEntry entry, void *para)
{
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&id, &attr, entry, para);
    pthread_attr_destroy(&attr);

    return 0;
}

LMO_S32 createThread(ThreadEntry entry, void* para, pthread_t *pid, int priority)
{
    pthread_attr_t attr;
    pthread_attr_t *pAttr;
    struct sched_param schedParam;
    pthread_attr_init(&attr);

    if(priority > 0)
    {
        schedParam.sched_priority = priority;
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        pthread_attr_setschedparam(&attr, &schedParam);
        pAttr = &attr;
    }
    else
    {
        pAttr = NULL;
    }

    if(0 == pthread_create(pid, pAttr, entry, para))//创建线程
    {
        pthread_attr_destroy(&attr);
        return 0;
    }

    pthread_attr_destroy(&attr);
    return -1;
}

LMO_S32 createNormalThread(ThreadEntry entry, void *para, pthread_t *pid)
{
    pthread_t ThreadId;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(0 == pthread_create(&ThreadId, &attr, entry, para))
    {
        pthread_attr_destroy(&attr);
//        TRY_EVALUATE_POINTER(pid, ThreadId);
        if(pid){
            *pid = ThreadId;
        }

        return 0;
    }

    pthread_attr_destroy(&attr);
    return -1;
}

/*********************************************************************
Function:
Description:
    执行系统命令
Example:
    SYSTEM("ls");
parameter:
    *cmdstring :命令内容
Return:
********************************************************************/
LMO_S32 SYSTEM(const char * cmdstring)
{
    pid_t pid;
    int status;

    if(cmdstring == NULL)
    {
        return (1);
    }

    if((pid = vfork())<0)
    {
        status = -1;
    }
    else if(pid == 0)	//子进中运行脚本命令
    {
        execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
        exit(127); //子进程正常执行则不会执行此语句
    }
    else		//父进程中等待子进程返回
    {
        while(waitpid(pid, &status, 0) < 0)
        {
            if(errno != EINTR)
            {
                status = -1;
                break;
            }
        }
    }
    return status;
}

/*********************************************************************
Function:
Description:
    执行系统命令，并把标准IO返回到程序
Example:
    char result[512] = {0};
    exec_cmd("ls", result);
parameter:
    *cmd    :命令内容
    *result :用作返回标准IO内容的字符串
    结果：*result == "EAI-libs  EAI-source  LICENSE  README_CN.md  README.md";
Return:
********************************************************************/
LMO_VOID exec_cmd(const char *cmd, char *result)
{
    char buf_ps[1024];
    char ps[1024]={0};
    FILE *ptr;
    strcpy(ps, cmd);
    if((ptr=popen(ps, "r"))!=NULL) {
        while(fgets(buf_ps, 1024, ptr)!=NULL) {
//	       可以通过这行来获取shell命令行中的每一行的输出
//	   	   printf("%s", buf_ps);
           strcat(result, buf_ps);
           if(strlen(result)>1024)
               break;
        }
        pclose(ptr);
        ptr = NULL;
    } else {
        printf("popen %s error\n", ps);
    }
}
