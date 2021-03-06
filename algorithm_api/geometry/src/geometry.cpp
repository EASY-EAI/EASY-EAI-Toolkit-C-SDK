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

//===========================================system===========================================
#include <sys/time.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <iconv.h>
#include <fcntl.h>
#include <dirent.h>
#include <dirent.h>
#include <semaphore.h>

//=========================================== C++ ===========================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <typeinfo>

//======================================= geometry =======================================
#include "geometry.h"

#define min(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y);(void) (&_x == &_y); _x<=_y ? _x : _y;})
#define max(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y);(void) (&_x == &_y); _x>=_y ? _x : _y;})

# if 0
#define min(x,y) ({x <= y ? x : y;})
#define max(x,y) ({x >= y ? x : y;})

/* IoU(Intersection over Union)
 * 
 */
float CalculateIoU(fRect_t rect1, fRect_t rect2)
{
    float w = fmax(0.f, fmin(rect1.right,  rect2.right)  - fmax(rect1.left, rect2.left) + 1.0);
    float h = fmax(0.f, fmin(rect1.bottom, rect2.bottom) - fmax(rect1.top,  rect2.top)  + 1.0);
    float i = w * h;
    float u = (rect1.right - rect1.left + 1.0) * (rect1.bottom - rect1.top + 1.0) + (rect2.right - rect2.left + 1.0) * (rect2.bottom - rect2.top + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}

float CalculateIntersection(fRect_t rect1, fRect_t rect2)
{
    float w = fmax(0.f, fmin(rect1.right,  rect2.right)  - fmax(rect1.left, rect2.left) + 1.0);
    float h = fmax(0.f, fmin(rect1.bottom, rect2.bottom) - fmax(rect1.top,  rect2.top)  + 1.0);
    float i = w * h;
    float u = (rect1.right - rect1.left + 1.0) * (rect1.bottom - rect1.top + 1.0) + (rect2.right - rect2.left + 1.0) * (rect2.bottom - rect2.top + 1.0) - i;
    return u <= 0.f ? 0.f : (i / u);
}
#endif

/* ????????????????????????????????????
 * 
 */
bool point_in_rect(s32Point_t point, s32Rect_t rect)
{
	// (<)  -- ????????????????????????
	// (<=) -- ???????????????????????????
	if((point.x < rect.left)||(rect.right < point.x))
		return false;
	
	if((point.y < rect.top)||(rect.bottom < point.y))
		return false;
	
	return true;
}

/* ???????????????????????????
 * 
 */
int32_t calc_rect_square(s32Rect_t rect)
{
	return (rect.right - rect.left)*(rect.bottom - rect.top);
}

/* ?????????????????????????????????
 * 
 */
s32Rect_t min_rect(s32Rect_t rect1, s32Rect_t rect2)
{
	if(calc_rect_square(rect1) <= calc_rect_square(rect2)){
		return rect1;
	}else{
		return rect2;
	}
}

/* ?????????????????????????????????
 * 
 */
s32Rect_t max_rect(s32Rect_t rect1, s32Rect_t rect2)
{
	if(calc_rect_square(rect2) >= calc_rect_square(rect1)){
		return rect2;
	}else{
		return rect1;
	}
}

/* ??????????????????????????????????????????
 * 
 */
bool rect_is_intersect(s32Rect_t rect1, s32Rect_t rect2)
{
	bool bIsPointRect = false;
	s32Point_t point;
	
// ================================================== //

	// ??????[??????A(?????????)]????????? [??????B???]
	point.x = rect1.left;
	point.y = rect1.top;
	bIsPointRect = point_in_rect(point, rect2);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????A(?????????)]????????? [??????B???]
	point.x = rect1.right;
	point.y = rect1.top;
	bIsPointRect = point_in_rect(point, rect2);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????A(?????????)]????????? [??????B???]
	point.x = rect1.right;
	point.y = rect1.bottom;
	bIsPointRect = point_in_rect(point, rect2);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????A(?????????)]????????? [??????B???]
	point.x = rect1.left;
	point.y = rect1.bottom;
	bIsPointRect = point_in_rect(point, rect2);
	if(bIsPointRect){return bIsPointRect;}
	
// ================================================== //
	
	// ??????[??????B(?????????)]????????? [??????A???]
	point.x = rect2.left;
	point.y = rect2.top;
	bIsPointRect = point_in_rect(point, rect1);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????B(?????????)]????????? [??????A???]
	point.x = rect2.right;
	point.y = rect2.top;
	bIsPointRect = point_in_rect(point, rect1);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????B(?????????)]????????? [??????A???]
	point.x = rect2.right;
	point.y = rect2.bottom;
	bIsPointRect = point_in_rect(point, rect1);
	if(bIsPointRect){return bIsPointRect;}
	
	// ??????[??????B(?????????)]????????? [??????A???]
	point.x = rect2.left;
	point.y = rect2.bottom;
	bIsPointRect = point_in_rect(point, rect1);
	if(bIsPointRect){return bIsPointRect;}
	
// ================================================== //
	
	return bIsPointRect;
}

/* ??????????????????????????????????????????
 * 
 */
int32_t calc_rect_intersect_square(s32Rect_t rect1, s32Rect_t rect2)
{
	int32_t w, h;
	// ????????????????????????
	if(rect_is_intersect(rect1, rect2)){
		w = min(rect1.right,  rect2.right)  - max(rect1.left, rect2.left);
		h = min(rect1.bottom, rect2.bottom) - max(rect1.top,  rect2.top);
		return w*h;
	}else{
		return 0;
	}
	
}

/* ???????????????[???????????????????????????]???[???????????????]??????
 * 
 */
double calc_intersect_of_min_rect(s32Rect_t rect1, s32Rect_t rect2)
{
	s32Rect_t minRect = min_rect(rect1, rect2);
	double Si = (double)calc_rect_intersect_square(rect1, rect2);
	double Sr = (double)calc_rect_square(minRect);
	
	return (Si/Sr);
}

/* ????????????????????????????????????
 * 
 */
double calc_intersect_of_union(s32Rect_t rect1, s32Rect_t rect2)
{
	double S1 = (double)calc_rect_square(rect1);
	double S2 = (double)calc_rect_square(rect2);
	double Si = (double)calc_rect_intersect_square(rect1, rect2);
	
	return (Si/(S1+S2-Si));
}
