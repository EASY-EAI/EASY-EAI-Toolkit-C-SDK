#ifndef __CONFIG_H__
#define __CONFIG_H__

//debug
#define DEBUG 1
#if DEBUG
#define	Debug(_FMT_,args...) printf("%s[f:%s]-(line:%d)---"#_FMT_"\n",__FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define Debug(_FMT_,args...) ;
#endif


//platform
#define LMO_TRUE 1
#define LMO_FALSE 0

#define BIT0  (0x0001 << 0 )
#define BIT1  (0x0001 << 1 )
#define BIT2  (0x0001 << 2 )
#define BIT3  (0x0001 << 3 )
#define BIT4  (0x0001 << 4 )
#define BIT5  (0x0001 << 5 )
#define BIT6  (0x0001 << 6 )
#define BIT7  (0x0001 << 7 )
#define BIT8  (0x0001 << 8 )
#define BIT9  (0x0001 << 9 )
#define BIT10 (0x0001 << 10)
#define BIT11 (0x0001 << 11)
#define BIT12 (0x0001 << 12)
#define BIT13 (0x0001 << 13)
#define BIT14 (0x0001 << 14)
#define BIT15 (0x0001 << 15)

typedef bool LMO_BOOL;
typedef void LMO_VOID;

typedef long long LMO_S64;
typedef int       LMO_S32;
typedef short     LMO_S16;
typedef char      LMO_S8;

typedef unsigned long long LMO_U64;
typedef unsigned int       LMO_U32;
typedef unsigned short     LMO_U16;
typedef unsigned char      LMO_U8;

typedef struct {LMO_S32 x; LMO_S32 y; LMO_S32 w; LMO_S32 h;}LMO_RECT;
typedef struct {LMO_U16 left; LMO_U16 top; LMO_U16 right; LMO_U16 bottom;}LMO_SPACE;

enum week_t{
	WK_SUNDAY,
	WK_MONDAY,
	WK_TUESDAY,
	WK_WEDNESDAY,
	WK_THURSDAY,
	WK_FRIDAY,
	WK_SATURDAY,
};

typedef struct {
    LMO_U16 year;
    LMO_U16 month;
    LMO_U16 day;
    LMO_U16 hour;
    LMO_U16 minute;
    LMO_U16 second;
}LMO_TIME;

#endif/*__CONFIG_H__*/

