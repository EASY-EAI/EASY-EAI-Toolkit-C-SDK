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
  
#ifndef PRINT_MSG_H
#define PRINT_MSG_H
  
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

namespace SysOpt {

/** 全局变量说明：
 * 此处只对该变量进行一个声明。
 * 定义：需要在调用set_print_callback接口的地方进行定义
**/
extern int32_t (*gpF_outlog)(char const *filePath, int lineNum, char const *funcName, int logLevel, char const *logCon, va_list args);
/* 接口说明：
 * [简写]-- void set_print_callback(函数指针);
 * [作用]-- 用于设置该Toolikit接口的内部打印输出回调函数
 * 
 * [打印回调]：(作用)用于输出打印到日志系统，以便统一管理；(注意)返回值以及参数列表必须与“函数指针”保持一致。
 * [函数指针]：int32_t (*)(char const *filePath, int lineNum, char const *funcName, int logLevel, char const *logCon, va_list args)
 * [使用样例]：
 *  int32_t printLog(char const *filePath, int lineNum, char const *funcName, int logLevel, char const *logCon, va_list args)
 *  {
 *      return 0;
 *  }
 *  int32_t main(void)
 *  {
 *      setXXXX_printf(printLog); //内部调用本接口(set_print_callback)
 *      return 0;
 *  }
**/
inline void set_print_callback(int32_t (* pFunc)(char const *filePath, int lineNum, char const *funcName, int logLevel, char const *logCon, va_list args))
{
    gpF_outlog = pFunc;
}
inline int32_t printMsg(char const *filePath, int lineNum, char const *funcName, int logLevel, char const *logCon, ...)
{
    int32_t ret = -1;
    if(gpF_outlog){
        va_list args;
        va_start(args, logCon); //把参数列表栈指针定位到logCon上
        ret = gpF_outlog(filePath, lineNum, funcName, logLevel, logCon, args);
        //va_end(args);
    }
    return ret;
}

}

// 定义打印级别
#define PRINT_LEVEL_NONE    0       // 不输出打印信息
#define PRINT_LEVEL_ERROR   1       // 错误信息, 否定判断时添加
#define PRINT_LEVEL_DEBUG   2       // 调试信息, 调试时添加
#define PRINT_LEVEL_TRACE   3       // 堆栈跟踪, 跟踪时添加
  
#define PRINT_TRACE(str, args...) \
      SysOpt::printMsg(__FILE__, __LINE__, __FUNCTION__, PRINT_LEVEL_TRACE, str, ##args)
#define PRINT_DEBUG(str, args...) \
      SysOpt::printMsg(__FILE__, __LINE__, __FUNCTION__, PRINT_LEVEL_DEBUG, str, ##args)
#define PRINT_ERROR(str, args...) \
      SysOpt::printMsg(__FILE__, __LINE__, __FUNCTION__, PRINT_LEVEL_ERROR, str, ##args)


#endif // PRINT_MSG_H
  
