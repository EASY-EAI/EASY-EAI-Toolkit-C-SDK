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
    
// ���ݱ�ϵͳ�ľ����ֽ�����Ĵ�Ÿ�ʽ
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
    snd_pcm_t *handle; //PCM�豸�������
    snd_pcm_format_t format; //���ݸ�ʽ

    uint16_t channels;
    size_t bits_per_sample; //һ����������������ȣ�8:8λ���ȡ�16:16λ���ȣ�
    size_t bytes_per_frame; //һ֡��Ƶ����ռ�õ��ڴ��С

    //ע�⣺snd_pcm_uframes_t��frameΪ��λ����������ByteΪ��λ��[1frame = (������*����λ��/8)Byte]
    snd_pcm_uframes_t frames_per_period; //һ�������ڵ�frame����
    snd_pcm_uframes_t frames_per_buffer; //ϵͳbuffer��frame����

    uint8_t *period_buf; //��Ŵ�WAV�ļ��ж�ȡ��һ�����ڵ�����

}pcm_container;


// ����WAV��ʽ����
static void set_wav_params(pcm_container *sound, uint32_t sampleRate, uint16_t channels, snd_pcm_format_t pcm_format)
{
    // 1: ���岢����һ��Ӳ�������ռ�
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);

    // 2: ��ʼ��Ӳ�������ռ�
    snd_pcm_hw_params_any(sound->handle, hwparams);

    // 3: ���÷���ģʽΪ����ģʽ����֡����ģʽ��
    snd_pcm_hw_params_set_access(sound->handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);

    // 4: ������������
    //snd_pcm_format_t pcm_format=SND_PCM_FORMAT_S16_LE;
    snd_pcm_hw_params_set_format(sound->handle, hwparams, pcm_format);
    sound->format = pcm_format;

    // 5: ����������Ŀ
    snd_pcm_hw_params_set_channels(sound->handle, hwparams, LE_SHORT(channels));
    sound->channels = LE_SHORT(channels);

    // 6: ���ò���Ƶ��
    // ע�⣺���ձ����õ�Ƶ�ʱ��������exact_rate��
    uint32_t exact_rate = LE_INT(sampleRate);
    snd_pcm_hw_params_set_rate_near(sound->handle, hwparams, &exact_rate, 0);

    // 7: ����buffer sizeΪ����֧�ֵ����ֵ
    snd_pcm_uframes_t buffer_size;
    snd_pcm_hw_params_get_buffer_size_max(hwparams, &buffer_size);
    snd_pcm_hw_params_set_buffer_size_near(sound->handle, hwparams, &buffer_size);

    // 8: ����buffer size����period size
    snd_pcm_uframes_t period_size = buffer_size / 4;
    snd_pcm_hw_params_set_period_size_near(sound->handle, hwparams, &period_size, 0);

    // 9: ��ȡbuffer size��period size
    // ע�⣺snd_pcm_uframes_t����frameΪ��λ����������ByteΪ��λ[1frame = (������*����λ��/8)Byte]��
    snd_pcm_hw_params_get_buffer_size(hwparams, &sound->frames_per_buffer);
    snd_pcm_hw_params_get_period_size(hwparams, &sound->frames_per_period, 0);

    // 10: ��װ��ЩPCM�豸����
    snd_pcm_hw_params(sound->handle, hwparams);

    // 11: ����һЩ����
    sound->bits_per_sample = snd_pcm_format_physical_width(pcm_format);
    sound->bytes_per_frame = sound->bits_per_sample/8 * channels;

    // 12: ����һ���������ݿռ�
    sound->period_buf = (uint8_t *)calloc(1, sound->frames_per_period * sound->bytes_per_frame);
}

pcm_container *g_pAudioInSound = NULL;

// ��PCM�豸�С���ȡframes֡����������:sound->period_buf����
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
    
    // 1: ������������
    g_pAudioInSound = calloc(1, sizeof(pcm_container));

    // 2: ������
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
    
    // 1: ������������
    g_pAudioIOutSound = calloc(1, sizeof(pcm_container));

    // 2: ������
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
    
    // ���������������ݺ�ʣ�಻��һ�����ڵ�����
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // ���ϴ�ʣ�࣬�Լ����������������װ��һ�������
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;
    
    
    // 1. *���ϴ�ʣ������ݺͱ��������������װ��һ��
    beWrittenDataLen = incompleteDataLen + dataLen;
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // ���ϴ�ʣ��������ͷţ�����ʼ��
            free(pIncompleteData);
            pIncompleteData = NULL;
            incompleteDataLen = 0;
            
            p = pBeWrittenData;
        }else{
            printf("create beWrittenData faild !\n");
            return -1;
        }
    }

    // 2. *����(����)��������������ݣ�����ԭ��
    //    *  �ܹ�1�������ϵ����ھͲ����������ڣ��ж�ľ��ܵ��´Ρ�
    //    *  �ϴ�����+������������ܲ���1�����ڡ��ͼ������´�������������ֱ����1�������ϵ����ھͲ��ų�����
    //    *  �ϴ�����+������������ܲ���1�����ڡ����������һ���������ˣ��Ǿ����ܳ��������ݺ��油�������ݣ����뵽һ���������ڣ������ų�����
    
    // һ�����ڵ��ڴ��С
    size_t periodBufSize = g_pAudioIOutSound->frames_per_period * g_pAudioIOutSound->bytes_per_frame;
    // ���������ӵ�еĲ������ڸ�������ʾ���������n�����ڡ�
    int periodNum = beWrittenDataLen/periodBufSize;
    incompleteDataLen = beWrittenDataLen - periodNum*periodBufSize;
    int i = 0;
    ssize_t nFrame = 0, sumFrame = 0;
    uint8_t *pPeriodBuf = g_pAudioIOutSound->period_buf;
    // ���ݺ���n����������
    if(0 < periodNum){
        for(i = 0; i < periodNum; i++){
            // ��ÿ�����ڵ����ݿ���������
            memcpy(pPeriodBuf, p + i*periodBufSize, periodBufSize);
            // �����������
            nFrame = write_pcm_to_device(g_pAudioIOutSound, pPeriodBuf);
            sumFrame += nFrame;
        }
    }
    // ���β�����в���һ�����ڵ�����
    if(0 < incompleteDataLen){
        // ���������һ������
        if(bEOS){
            // ���Ȱ����ݿ�������
            memcpy(pPeriodBuf, p + i*periodBufSize, incompleteDataLen);
            // Ȼ����period_buf�ĺ��β���һЩ�������ݣ��Բ���һ�����ڡ�
            snd_pcm_uframes_t frames = (periodBufSize - incompleteDataLen)/g_pAudioIOutSound->bytes_per_frame;
            snd_pcm_format_set_silence(g_pAudioIOutSound->format, pPeriodBuf + incompleteDataLen, frames);
            // ���������ڲ��ų���
            nFrame = write_pcm_to_device(g_pAudioIOutSound, pPeriodBuf);
            sumFrame += nFrame;
        
        // �������һ�������������ǾͰ�ʣ�����ݱ����������ȴչ���һ�������ٲ���
        }else{
            pIncompleteData = (uint8_t *)malloc(incompleteDataLen);
            if(pIncompleteData){
                memcpy(pIncompleteData, p + i*periodBufSize, incompleteDataLen);
            }
        }
    }
    
    // 3. *�ѱ��ε���ת�������
    if(pBeWrittenData){
        free(pBeWrittenData);   //delete[] pBeWrittenData;
        pBeWrittenData = NULL;
        beWrittenDataLen = 0;
    }
    
    return sumFrame * g_pAudioIOutSound->bytes_per_frame;
}

#else   // ���ַ���ռ�õ��ڴ��CPU�����ϴ󣬲��Ƽ�ʹ�ã���ɾ����������ο�
    #if 1
static ssize_t write_pcm_period(pcm_container *sound, uint8_t *pData, int32_t frameNum, bool bEOS)
{
    // ���������������ݺ�ʣ�಻��һ�����ڵ�����
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // ���ϴ�ʣ�࣬�Լ����������������װ��һ�������
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;
    
    uint8_t *p = pData;
    uint32_t dataLen = frameNum*sound->bytes_per_frame;
    beWrittenDataLen = incompleteDataLen + dataLen;
    // ���ϴ�ʣ������ݺͱ��������������װ��һ��
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // ���ϴ�ʣ��������ͷţ�����ʼ��
            free(pIncompleteData);
            pIncompleteData = NULL;
            incompleteDataLen = 0;
            
            p = pBeWrittenData;
        }else{
            printf("create beWrittenData faild !\n");
            return -1;
        }
    }

    // һ�����ڵ��ڴ��С
    size_t periodBufSize = sound->frames_per_period * sound->bytes_per_frame;
    // ���������ӵ�еĲ������ڸ�������ʾ���������n�����ڡ�
    int periodNum = beWrittenDataLen/periodBufSize;
    incompleteDataLen = beWrittenDataLen - periodNum*periodBufSize;
    int i = 0;
    ssize_t nFrame = 0, sumFrame = 0;
    uint8_t *pPeriodBuf = sound->period_buf;
    // ���ݺ���n����������
    if(0 < periodNum){
        for(i = 0; i < periodNum; i++){
            // ��ÿ�����ڵ��ڴ濽��������
            memcpy(pPeriodBuf, p + i*periodBufSize, periodBufSize);
            // �����������
            nFrame = write_pcm_to_device(sound, pPeriodBuf);
            sumFrame += nFrame;
        }
    }
    // ���β�����в���һ�����ڵ�����
    if(0 < incompleteDataLen){
        // ���������һ��������
        if(bEOS){
            // ���Ȱ����ݿ�������
            memcpy(pPeriodBuf, p + i*periodBufSize, incompleteDataLen);
            // Ȼ����period_buf�ĺ��β���һЩ�������ݣ��Բ���һ�����ڡ�
            snd_pcm_format_set_silence(sound->format, pPeriodBuf + incompleteDataLen, (periodBufSize - incompleteDataLen)/sound->bytes_per_frame);
            //memset(pPeriodBuf + incompleteDataLen, 0, periodBufSize - incompleteDataLen);
            // ����������ڲ��ų���
            nFrame = write_pcm_to_device(sound, pPeriodBuf);
            sumFrame += nFrame;
        
        // �������һ���������ǾͰ�ʣ�����ݱ����������ȴչ���һ�������ٲ���
        }else{
            pIncompleteData = (uint8_t *)malloc(incompleteDataLen);
            if(pIncompleteData){
                memcpy(pIncompleteData, p + i*periodBufSize, incompleteDataLen);
            }
        }
    }
    
    // �ѱ��ε���ת�������
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
        if(nFrame <= 0){ //82��
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
    // ��������֡���ݺ�ʣ�಻��һ��֡������
    static uint8_t *pIncompleteData = NULL;
    static int32_t incompleteDataLen = 0;

    // ���ϴ�ʣ�࣬�Լ����������������װ��һ�������
    static uint8_t *pBeWrittenData = NULL;
    static int32_t beWrittenDataLen = 0;

    uint8_t *p = pData;
    beWrittenDataLen = incompleteDataLen + dataLen;
    // ���ϴ�ʣ������ݺͱ��������������װ��һ��
    if((0 < incompleteDataLen) && (pIncompleteData)){
        pBeWrittenData = (uint8_t *)malloc(beWrittenDataLen);   // new uint8_t[beWrittenDataLen];
        if(pBeWrittenData){
            memcpy(pBeWrittenData, pIncompleteData, incompleteDataLen);
            memcpy(pBeWrittenData+incompleteDataLen, pData, dataLen);

            // ���ϴ�ʣ��������ͷţ�����ʼ��
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
    // д������֡�����ݵ�����
    int writtenFrameNum =  write_pcm_period(g_pAudioIOutSound, p, frameNum, bEOS);
    // ��ε�����β����ʣ�಻��һ��֡�����ݣ����Ҳ������һ���������ǾͰ�ʣ�����ݱ�������
    if((0 < incompleteDataLen)&&(false == bEOS)){
        pIncompleteData = (uint8_t *)malloc(incompleteDataLen);    // new uint8_t[incompleteDataLen];
        if(pIncompleteData){
            memcpy(pIncompleteData, p+frameNum*g_pAudioIOutSound->bytes_per_frame, incompleteDataLen);
        }
    }

    // �ѱ��ε���ת�������
    if(pBeWrittenData){
        free(pBeWrittenData);   //delete[] pBeWrittenData;
        pBeWrittenData = NULL;
        beWrittenDataLen = 0;
    }
    
    return writtenFrameNum * g_pAudioIOutSound->bytes_per_frame;
}
#endif

