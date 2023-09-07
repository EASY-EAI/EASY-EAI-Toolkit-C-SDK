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
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "audio.h"
    
// 根据本系统的具体字节序处理的存放格式
#if __BYTE_ORDER == __LITTLE_ENDIAN
    
    #define RIFF ('F'<<24|'F'<<16|'I'<<8|'R'<<0)
    #define WAVE ('E'<<24|'V'<<16|'A'<<8|'W'<<0)
    #define FMT  (' '<<24|'t'<<16|'m'<<8|'f'<<0)
    #define DATA ('a'<<24|'t'<<16|'a'<<8|'d'<<0)
        
    #define LE_SHORT(val) (val)
    #define LE_INT(val)   (val)
        
#elif __BYTE_ORDER == __BIG_ENDIAN
    
    #define RIFF ('R'<<24|'I'<<16|'F'<<8|'F'<<0)
    #define WAVE ('W'<<24|'A'<<16|'V'<<8|'E'<<0)
    #define FMT  ('f'<<24|'m'<<16|'t'<<8|' '<<0)
    #define DATA ('d'<<24|'a'<<16|'t'<<8|'a'<<0)
    
    #define LE_SHORT(val) bswap_16(val)
    #define LE_INT(val)   bswap_32(val)
        
#endif

#define MIN(a, b) \
    ({ \
        typeof(a) _a = a; \
        typeof(b) _b = b; \
        (void)(_a == _b); \
        _a < _b ? _a : _b; \
    })

typedef struct
{
    snd_pcm_t *handle; //PCM设备操作句柄
    snd_pcm_format_t format; //数据格式

    uint16_t channels;
    size_t bits_per_sample; //一个采样点的量化精度（8:8位精度、16:16位精度）
    size_t bytes_per_frame; //一帧音频数据占用的内存大小

    //注意：snd_pcm_uframes_t以frame为单位，而不是以Byte为单位。[1frame = (声道数*量化位深/8)Byte]
    snd_pcm_uframes_t frames_per_period; //一个周期内的frame个数
    snd_pcm_uframes_t frames_per_buffer; //系统buffer的frame个数

    uint8_t *period_buf; //存放从WAV文件中读取的一个周期的数据

}pcm_container;


// 设置WAV格式参数
static void set_wav_params(pcm_container *sound, uint32_t sampleRate, uint16_t channels, snd_pcm_format_t pcm_format)
{
    // 1: 定义并分配一个硬件参数空间
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);

    // 2: 初始化硬件参数空间
    snd_pcm_hw_params_any(sound->handle, hwparams);

    // 3: 设置访问模式为交错模式（即帧连续模式）
    snd_pcm_hw_params_set_access(sound->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    // 4: 设置量化参数
    //snd_pcm_format_t pcm_format=SND_PCM_FORMAT_S16_LE;
    snd_pcm_hw_params_set_format(sound->handle, hwparams, pcm_format);
    sound->format = pcm_format;

    // 5: 设置音轨数目
    snd_pcm_hw_params_set_channels(sound->handle, hwparams, LE_SHORT(channels));
    sound->channels = LE_SHORT(channels);

    // 6: 设置采样频率
    // 注意：最终被设置的频率被存放在来exact_rate中
    uint32_t exact_rate = LE_INT(sampleRate);
    snd_pcm_hw_params_set_rate_near(sound->handle, hwparams, &exact_rate, 0);

    // 7: 设置buffer size为声卡支持的最大值
    snd_pcm_uframes_t buffer_size;
    snd_pcm_hw_params_get_buffer_size_max(hwparams, &buffer_size);
    snd_pcm_hw_params_set_buffer_size_near(sound->handle, hwparams, &buffer_size);

    // 8: 根据buffer size设置period size
    snd_pcm_uframes_t period_size = buffer_size / 4;
    snd_pcm_hw_params_set_period_size_near(sound->handle, hwparams, &period_size, 0);

    // 9: 获取buffer size和period size
    // 注意：snd_pcm_uframes_t均以frame为单位，而不是以Byte为单位[1frame = (声道数*量化位深/8)Byte]。
    snd_pcm_hw_params_get_buffer_size(hwparams, &sound->frames_per_buffer);
    snd_pcm_hw_params_get_period_size(hwparams, &sound->frames_per_period, 0);

    // 10: 安装这些PCM设备参数
    snd_pcm_hw_params(sound->handle, hwparams);

    // 11: 保存一些参数
    sound->bits_per_sample = snd_pcm_format_physical_width(pcm_format);
    sound->bytes_per_frame = sound->bits_per_sample/8 * channels;

    // 12: 分配一个周期数据空间
    sound->period_buf = (uint8_t *)calloc(1, sound->frames_per_period * sound->bytes_per_frame);
}

pcm_container *g_pAudioInSound = NULL;

// 从PCM设备中【读取frames帧】到【缓存:sound->period_buf】中
static snd_pcm_uframes_t read_pcm_data(pcm_container *sound, snd_pcm_uframes_t frames)
{
    snd_pcm_uframes_t exact_frames = 0;
    snd_pcm_uframes_t n = 0;

    uint8_t *p = sound->period_buf;
    while (0 < frames) {
        n = snd_pcm_readi(sound->handle, p, frames);

        frames -= n;
        exact_frames += n;
        p += (n * sound->bytes_per_frame);
    }

    return exact_frames;
}

int32_t ai_init(uint32_t sampleRate, uint16_t channels, snd_pcm_format_t fmt)
{
    if(g_pAudioInSound){
        ai_exit();
    }
    
    // 1: 创建声卡对象
    g_pAudioInSound = calloc(1, sizeof(pcm_container));

    // 2: 打开声卡
    int ret = snd_pcm_open(&g_pAudioInSound->handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if(ret != 0) {
        perror("snd_pcm_open() failed");
        return -1;
    }

    set_wav_params(g_pAudioInSound, sampleRate, channels, fmt);

    return 0;
}

int32_t ai_exit(void)
{
    if(g_pAudioInSound){
        if(g_pAudioInSound->period_buf){
            free(g_pAudioInSound->period_buf);
            g_pAudioInSound->period_buf = NULL;
        }
        
        snd_pcm_drain(g_pAudioInSound->handle);
        snd_pcm_close(g_pAudioInSound->handle);
        free(g_pAudioInSound);

        g_pAudioInSound = NULL;
    }

    return 0;
}

uint32_t ai_pcmPeriodSize(void)
{
    if(g_pAudioInSound){
        return (uint32_t)g_pAudioInSound->frames_per_period*g_pAudioInSound->bytes_per_frame;
    }else{
        return 0;
    }
}

uint32_t ai_readpcmData(uint32_t dataLen)
{
    if(g_pAudioInSound){
        snd_pcm_uframes_t needRead_frams = dataLen / (g_pAudioInSound->bytes_per_frame);
       
        snd_pcm_uframes_t total_frams = needRead_frams;
        while(0 < total_frams) {
            snd_pcm_uframes_t n = MIN(total_frams, g_pAudioInSound->frames_per_period);
            snd_pcm_uframes_t frames_read = read_pcm_data(g_pAudioInSound, n);
            total_frams -= frames_read;

        }

        return (uint32_t)needRead_frams*g_pAudioInSound->bytes_per_frame;
    }else{
        return 0;
    }
}

uint8_t *ai_pcmBufptr(void)
{
    if((NULL != g_pAudioInSound) && (NULL != g_pAudioInSound->period_buf)){
        return g_pAudioInSound->period_buf;
    }else{
        return NULL;
    }
}


pcm_container *g_pAudioIOutSound = NULL;
static ssize_t write_pcm_to_device(pcm_container *sound, uint8_t *pData)
{
    ssize_t nFrame = snd_pcm_writei(sound->handle, pData, sound->frames_per_period);
    if(nFrame <= 0){
        fprintf(stderr, "Error snd_pcm_writei: [%s]\n", snd_strerror(nFrame));
        return 0;
    }else{
        return nFrame;
    }
}
int32_t ao_init(uint32_t sampleRate, uint16_t channels, snd_pcm_format_t fmt)
{
    if(g_pAudioIOutSound){
        ao_exit();
    }
    
    // 1: 创建声卡对象
    g_pAudioIOutSound = calloc(1, sizeof(pcm_container));

    // 2: 打开声卡
    int ret = snd_pcm_open(&g_pAudioIOutSound->handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(ret != 0) {
        perror("snd_pcm_open() failed");
        return -1;
    }

    set_wav_params(g_pAudioIOutSound, sampleRate, channels, fmt);

    return 0;
}

int32_t ao_exit(void)
{
    if(g_pAudioIOutSound){
        if(g_pAudioIOutSound->period_buf){
            free(g_pAudioIOutSound->period_buf);
            g_pAudioIOutSound->period_buf = NULL;
        }
        
        snd_pcm_drain(g_pAudioIOutSound->handle);
        snd_pcm_close(g_pAudioIOutSound->handle);
        free(g_pAudioIOutSound);

        g_pAudioIOutSound = NULL;
    }

    return 0;
}

#if 1
int32_t ao_writepcmBuf(uint8_t *pData, uint32_t dataLen, bool bEOS)
{
    uint8_t *p = pData;
    
    // 除开完整周期数据后，剩余不够一个周期的数据
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // 把上次剩余，以及本次送入的数据组装在一起的数据
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;
    
    
    // 1. *把上次剩余的数据和本次送入的数据组装在一起
    beWrittenDataLen = incompleteDataLen + dataLen;
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // 把上次剩余的数据释放，并初始化
            free(pIncompleteData);
            pIncompleteData = NULL;
            incompleteDataLen = 0;
            
            p = pBeWrittenData;
        }else{
            printf("create beWrittenData faild !\n");
            return -1;
        }
    }

    // 2. *处理(播放)这次攒起来的数据，处理原则：
    //    *  攒够1个或以上的周期就播放完整周期，有多的就攒到下次。
    //    *  上次数据+本次数据如果攒不够1个周期。就继续跟下次数据攒起来，直到够1个或以上的周期就播放出来。
    //    *  上次数据+本次数据如果攒不够1个周期。并且是最后一次送数据了，那就在攒出来的数据后面补静音数据，补齐到一个完整周期，并播放出来。
    
    // 一个周期的内存大小
    size_t periodBufSize = g_pAudioIOutSound->frames_per_period * g_pAudioIOutSound->bytes_per_frame;
    // 这段数据所拥有的播放周期个数【表示这段数据有n个周期】
    int periodNum = beWrittenDataLen/periodBufSize;
    incompleteDataLen = beWrittenDataLen - periodNum*periodBufSize;
    int i = 0;
    ssize_t nFrame = 0, sumFrame = 0;
    uint8_t *pPeriodBuf = g_pAudioIOutSound->period_buf;
    // 数据含有n个播放周期
    if(0 < periodNum){
        for(i = 0; i < periodNum; i++){
            // 把每个周期的数据拷进缓存里
            memcpy(pPeriodBuf, p + i*periodBufSize, periodBufSize);
            // 播放这个周期
            nFrame = write_pcm_to_device(g_pAudioIOutSound, pPeriodBuf);
            sumFrame += nFrame;
        }
    }
    // 如果尾部还有不够一个周期的数据
    if(0 < incompleteDataLen){
        // 并且是最后一段数据
        if(bEOS){
            // 首先把数据拷进缓存
            memcpy(pPeriodBuf, p + i*periodBufSize, incompleteDataLen);
            // 然后在period_buf的后半段插入一些静音数据，以补齐一个周期。
            snd_pcm_uframes_t frames = (periodBufSize - incompleteDataLen)/g_pAudioIOutSound->bytes_per_frame;
            snd_pcm_format_set_silence(g_pAudioIOutSound->format, pPeriodBuf + incompleteDataLen, frames);
            // 最后这个周期播放出来
            nFrame = write_pcm_to_device(g_pAudioIOutSound, pPeriodBuf);
            sumFrame += nFrame;
        
        // 不是最后一次送数据流，那就把剩余数据保存起来，等凑够了一个周期再播放
        }else{
            pIncompleteData = (uint8_t *)malloc(incompleteDataLen);
            if(pIncompleteData){
                memcpy(pIncompleteData, p + i*periodBufSize, incompleteDataLen);
            }
        }
    }
    
    // 3. *把本次的中转数据清空
    if(pBeWrittenData){
        free(pBeWrittenData);   //delete[] pBeWrittenData;
        pBeWrittenData = NULL;
        beWrittenDataLen = 0;
    }
    
    return sumFrame * g_pAudioIOutSound->bytes_per_frame;
}

#else   // 这种方法占用的内存和CPU开销较大，不推荐使用，不删掉代码仅供参考
    #if 1
static ssize_t write_pcm_period(pcm_container *sound, uint8_t *pData, int32_t frameNum, bool bEOS)
{
    // 除开完整周期数据后，剩余不够一个周期的数据
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // 把上次剩余，以及本次送入的数据组装在一起的数据
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;
    
    uint8_t *p = pData;
    uint32_t dataLen = frameNum*sound->bytes_per_frame;
    beWrittenDataLen = incompleteDataLen + dataLen;
    // 把上次剩余的数据和本次送入的数据组装在一起
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // 把上次剩余的数据释放，并初始化
            free(pIncompleteData);
            pIncompleteData = NULL;
            incompleteDataLen = 0;
            
            p = pBeWrittenData;
        }else{
            printf("create beWrittenData faild !\n");
            return -1;
        }
    }

    // 一个周期的内存大小
    size_t periodBufSize = sound->frames_per_period * sound->bytes_per_frame;
    // 这段数据所拥有的播放周期个数【表示这段数据有n个周期】
    int periodNum = beWrittenDataLen/periodBufSize;
    incompleteDataLen = beWrittenDataLen - periodNum*periodBufSize;
    int i = 0;
    ssize_t nFrame = 0, sumFrame = 0;
    uint8_t *pPeriodBuf = sound->period_buf;
    // 数据含有n个播放周期
    if(0 < periodNum){
        for(i = 0; i < periodNum; i++){
            // 把每个周期的内存拷进缓存里
            memcpy(pPeriodBuf, p + i*periodBufSize, periodBufSize);
            // 播放这个周期
            nFrame = write_pcm_to_device(sound, pPeriodBuf);
            sumFrame += nFrame;
        }
    }
    // 如果尾部还有不够一个周期的数据
    if(0 < incompleteDataLen){
        // 并且是最后一段数据了
        if(bEOS){
            // 首先把数据拷进缓存
            memcpy(pPeriodBuf, p + i*periodBufSize, incompleteDataLen);
            // 然后在period_buf的后半段插入一些静音数据，以补齐一个周期。
            snd_pcm_format_set_silence(sound->format, pPeriodBuf + incompleteDataLen, (periodBufSize - incompleteDataLen)/sound->bytes_per_frame);
            //memset(pPeriodBuf + incompleteDataLen, 0, periodBufSize - incompleteDataLen);
            // 并把这个周期播放出来
            nFrame = write_pcm_to_device(sound, pPeriodBuf);
            sumFrame += nFrame;
        
        // 不是最后一次送流，那就把剩余数据保存起来，等凑够了一个周期再播放
        }else{
            pIncompleteData = (uint8_t *)malloc(incompleteDataLen);
            if(pIncompleteData){
                memcpy(pIncompleteData, p + i*periodBufSize, incompleteDataLen);
            }
        }
    }
    
    // 把本次的中转数据清空
    if(pBeWrittenData){
        free(pBeWrittenData);   //delete[] pBeWrittenData;
        pBeWrittenData = NULL;
        beWrittenDataLen = 0;
    }
    
    return sumFrame;
}
    #else
static ssize_t write_pcm_period(pcm_container *sound, uint8_t *pData, int32_t frameNum, bool bEOS)
{
    ssize_t nFrame;
    ssize_t sumFrame;
    
    while(0 < frameNum)
    {
        nFrame = snd_pcm_writei(sound->handle, pData, frameNum);        
        if(nFrame <= 0){ //82行
            fprintf(stderr, "Error snd_pcm_writei: [%s]\n", snd_strerror(nFrame));
            break;
        }
        if(nFrame > 0){
            frameNum -= nFrame;
            sumFrame += nFrame;
            pData += nFrame * sound->bytes_per_frame;
        }
    }

    return sumFrame;
}
    #endif
int32_t ao_writepcmBuf(uint8_t *pData, uint32_t dataLen, bool bEOS)
{
    // 除开完整帧数据后，剩余不够一个帧的数据
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // 把上次剩余，以及本次送入的数据组装在一起的数据
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;

    uint8_t *p = pData;
    beWrittenDataLen = incompleteDataLen + dataLen;
    // 把上次剩余的数据和本次送入的数据组装在一起
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);   // new uint8_t[beWrittenDataLen];
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // 把上次剩余的数据释放，并初始化
            free(pIncompleteData);  //delete[] pIncompleteData;
            pIncompleteData = NULL;
            incompleteDataLen = 0;
            
            p = pBeWrittenData;
        }else{
            printf("create beWrittenData faild !\n");
            return -1;
        }
    }
        
    int frameNum = beWrittenDataLen / g_pAudioIOutSound->bytes_per_frame;
    incompleteDataLen = beWrittenDataLen - frameNum*g_pAudioIOutSound->bytes_per_frame;
    // 写入完整帧的数据到声卡
    int writtenFrameNum =  write_pcm_period(g_pAudioIOutSound, p, frameNum, bEOS);
    // 这次的数据尾部有剩余不够一个帧的数据，并且不是最后一次送流，那就把剩余数据保存起来
    if((0 < incompleteDataLen)&&(false == bEOS)){
        pIncompleteData = (uint8_t *)malloc(incompleteDataLen);    // new uint8_t[incompleteDataLen];
        if(pIncompleteData){
            memcpy(pIncompleteData, p+frameNum*g_pAudioIOutSound->bytes_per_frame, incompleteDataLen);
        }
    }

    // 把本次的中转数据清空
    if(pBeWrittenData){
        free(pBeWrittenData);   //delete[] pBeWrittenData;
        pBeWrittenData = NULL;
        beWrittenDataLen = 0;
    }
    
    return writtenFrameNum * g_pAudioIOutSound->bytes_per_frame;
}
#endif

