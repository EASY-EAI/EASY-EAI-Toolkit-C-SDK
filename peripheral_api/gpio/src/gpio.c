/**
 *
 * Copyright 2023 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: ZJH <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <gpiod.h>

#include "gpio.h"

#define DEBUG_COLOR_TAIL "\033[0m"

typedef struct {
    char pinName[32];
    int  direction;
    int  defaultVal;
    int  fd; //用sysfs方式访问
    struct gpiod_line *line;    //用gpiod方式访问
    int  LLCount; //low  level count:捕抓到的低电平次数
    int  HLCount; //high level count:捕抓到的高电平次数
}GPIO_OBJ_t;

static int  g_pin_number = 0;
GPIO_OBJ_t *g_pinObj_array = NULL;
static int create_pinObj_array(int objNumber)
{
    if(objNumber <= 0)
        return -1;
    
    g_pinObj_array = (GPIO_OBJ_t *)malloc(objNumber * sizeof(GPIO_OBJ_t));
    if(!g_pinObj_array){
        return -1;
    }
    memset(g_pinObj_array, 0, objNumber * sizeof(GPIO_OBJ_t));
    
    return 0;
}
static void destory_pinObj_array()
{
    if(g_pinObj_array){
        free(g_pinObj_array);
        g_pinObj_array = NULL;
    }
}
static GPIO_OBJ_t *get_pinObj(int index){
    if((index < 0)||(g_pin_number <= index))
        return NULL;
    
    return (g_pinObj_array+index);
}
static int set_pinObj(int index, GPIO_OBJ_t *pObj){
    if((index < 0)||(g_pin_number <= index))
        return -1;
    if(NULL == pObj)
        return -1;
    
    memcpy(g_pinObj_array+index, pObj, sizeof(GPIO_OBJ_t));

    return 0;
}

#define GPIO_MAXGROUP_NUM 5
static struct gpiod_chip *g_GPIOChip[GPIO_MAXGROUP_NUM] = {NULL};
static int search_obj_index(const char *pinName)
{
    int index = -1;

    GPIO_OBJ_t *pinObj = NULL;
    for(int i = 0; i < g_pin_number; i++){
        pinObj = get_pinObj(i);
        if(pinObj){
            if(0 == strcmp(pinObj->pinName, pinName)){
                index = i;
                break;
            }
        }
    }
    return index;
}

static int analysis_pinName(const char *pinName, int *group, int *offset)
{
    // 合法性判断
    if(8 != strlen(pinName))
        return -1;
    if(('G' != pinName[0])||('P' != pinName[1])||('I' != pinName[2])||('O' != pinName[3]))
        return -1;
    if('_' != pinName[5])
        return -1;
    if(pinName[6] < 65)
        return -1;

    char cGroup[2]  = {0};
    cGroup[0]  = pinName[4];
    *group = atoi((const char *)cGroup);
    
    char cOffset[2] = {0};
    cOffset[0] = pinName[7];    
    *offset = 8*(pinName[6]-65) + atoi((const char *)cOffset);

    return 0;
}

static struct gpiod_line *pin_init_gpiod(const char *pinName, bool bIsOutput)
{
	int ret;

    int group, offset;
    if(analysis_pinName(pinName, &group, &offset) < 0){
        return NULL;
    }
    
    struct gpiod_line *line = NULL;
    line = gpiod_chip_get_line(g_GPIOChip[group], offset);
    if (!line) {
        perror("Get line failed\n");
        return line;
    }

    if(bIsOutput){
        ret = gpiod_line_request_output(line, pinName, 0);
    }else{
        ret = gpiod_line_request_input(line, pinName);
    }
    if (ret < 0) {
        perror("Request line  failed\n");
        gpiod_line_release(line);
        return NULL;
    }
    
	return line;
}

int pin_out_val(const char *pinName, int val)
{
    int index = search_obj_index(pinName);
    if(index < 0)
        return index;

    GPIO_OBJ_t *pinObj = get_pinObj(index);
    if(NULL == pinObj)
        return -1;
    
    if (gpiod_line_set_value(pinObj->line, val) < 0) {
        perror("Set line output failed\n");
        gpiod_line_release(pinObj->line);
        return -1;
    }
    return 0;
}

int read_pin_val(const char *pinName)
{
    int index = search_obj_index(pinName);
    if(index < 0)
        return index;
    
    GPIO_OBJ_t *pinObj = get_pinObj(index);
    if(NULL == pinObj)
        return -1;

    return gpiod_line_get_value(pinObj->line);
}

int gpio_init(const GPIOCfg_t gpioCfgs[], int arraySize)
{
    printf(">>>>> GPIO:\n");
    const char *strGPIOChip = "gpiochip";
    
    char chipName[32];
    for(int i = 0; i < GPIO_MAXGROUP_NUM; i++) {
        memset(chipName, 0, sizeof(chipName));
        sprintf(chipName, "%s%d", strGPIOChip, i);

        if(NULL == g_GPIOChip[i]){
            g_GPIOChip[i] = gpiod_chip_open_by_name(chipName);
        }
        if (!g_GPIOChip[i]) {
            perror("Open chip failed\n");
        }
    }

    gpio_uinit();
    g_pin_number = arraySize;
    if(create_pinObj_array(g_pin_number) < 0){
        printf("gpio obj init error !!!\n");
        return -1;
    }
    
    GPIO_OBJ_t pinObj;
    for(int i = 0; i < g_pin_number; i++) {
        printf("[%d]======(Name:%s, Dir:%s)  ", i, gpioCfgs[i].pinName, gpioCfgs[i].direction?"OUTPUT":"INPUT");
        memset(&pinObj, 0, sizeof(GPIO_OBJ_t));
        
        pinObj.line = pin_init_gpiod(gpioCfgs[i].pinName, gpioCfgs[i].direction);
        if(NULL == pinObj.line){
            printf("\t\033[33m【Init ERROR!】%s\n", DEBUG_COLOR_TAIL);
            continue;
        }
        memcpy(pinObj.pinName, gpioCfgs[i].pinName, strlen(gpioCfgs[i].pinName));
        pinObj.direction  = gpioCfgs[i].direction;
        pinObj.defaultVal = gpioCfgs[i].val;
        if(set_pinObj(i, &pinObj) < 0){
            printf("\t\033[33m【Save pin object ERROR!】%s\n", DEBUG_COLOR_TAIL);
            continue;
        }
        
        printf("\t\033[36m【Init OK ...】%s\n", DEBUG_COLOR_TAIL);
    }
    
    printf("-------------------------------------------------------------\n");
    return 0;
}

int gpio_uinit()
{
    GPIO_OBJ_t *pinObj;
    for(int i = 0; i < g_pin_number; i++){
        pinObj = get_pinObj(i);
        if(pinObj){
            gpiod_line_release(pinObj->line);
        }
        memset(pinObj->pinName, 0, sizeof(pinObj->pinName));
        pinObj->line = NULL;
        pinObj->LLCount = 0;
        pinObj->HLCount = 0;
    }

    g_pin_number = 0;
    destory_pinObj_array();

    return 0;
}

