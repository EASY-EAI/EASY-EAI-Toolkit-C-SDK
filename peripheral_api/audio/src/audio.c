/**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: CHM <chenhaiman@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rkmedia/rkmedia_api.h>

#define	AI_CHN_NUM	0
#define	AO_CHN_NUM	0

static AO_CHN_ATTR_S g_ao_attr;
static AI_CHN_ATTR_S g_ai_attr;
static int g_play_running = 0;
static int g_rec_running = 0;
static pthread_t g_play_thread_id;
static pthread_t g_rec_thread_id;

static void *rec_thread(void *arg)
{
    char *path = (void *)arg;
	FILE *fp = NULL;
	int ret = 0;
	MEDIA_BUFFER mb = NULL;

	g_rec_running = 1;
	fp = fopen(path, "w");
	if (!fp) {
		printf("ERROR: open %s failed!\n", path);
		g_rec_running = 0;
		ret = -1;
	}

	while (g_rec_running) {
		mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AI, AI_CHN_NUM, -1);
		if (!mb) {
	 		printf("RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
			break;
		}
	
		if (fp){
			fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), fp);
		}
		RK_MPI_MB_ReleaseBuffer(mb);
		mb = NULL;
	}
	if (fp)
    	fclose(fp);

	if(mb){
		RK_MPI_MB_ReleaseBuffer(mb);
		mb = NULL;
	}

	g_play_running = 0;
	return (void *)0;
}

static void *play_thread(void *arg)
{
    char *path = (void *)arg;
	FILE *fp = NULL;
	int ret = 0;
	MEDIA_BUFFER mb = NULL;
	RK_U32 u32Timeval = g_ao_attr.u32NbSamples * 1000000 / g_ao_attr.u32SampleRate; // us
	RK_U32 u32ReadSize = g_ao_attr.u32NbSamples * g_ao_attr.u32Channels * 2; // RK_SAMPLE_FMT_S16:2Bytes

	g_play_running = 1;
	fp = fopen(path, "r");
	if (!fp) {
		printf("ERROR: open %s failed!\n", path);
		g_play_running = 0;
		ret = -1;
	}
	while (g_play_running) {
		mb = RK_MPI_MB_CreateAudioBuffer(u32ReadSize, RK_FALSE);
		if (!mb) {
			printf("ERROR: malloc failed! size:%d\n", u32ReadSize);
			ret = -1;
			break;
		}
		ret = fread(RK_MPI_MB_GetPtr(mb), 1, u32ReadSize, fp);
		if (!ret) {
			printf("# Get end of file!\n");
			ret = 0;
			break;
		}
		RK_MPI_MB_SetSize(mb, u32ReadSize);
		ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AO, AO_CHN_NUM, mb);
		if (ret) {
			printf("ERROR: RK_MPI_SYS_SendMediaBuffer failed! ret = %d\n", ret);
			break;
		}
		usleep(u32Timeval);
		RK_MPI_MB_ReleaseBuffer(mb);
		mb = NULL;
	}

	if(mb){
		RK_MPI_MB_ReleaseBuffer(mb);
		mb = NULL;
	}
	g_play_running = 0;
	return (void *)0;

}

int ao_init(SAMPLE_FORMAT_E fmt, int rate, int chn, int vol)
{
	int ret = 0;
	RK_S32 s32CurrentVolmue = 0;

	g_ao_attr.pcAudioNode = "default";
	g_ao_attr.enSampleFormat = fmt;
	g_ao_attr.u32NbSamples = 1024;
	g_ao_attr.u32SampleRate = rate;
	g_ao_attr.u32Channels = chn;

	RK_MPI_SYS_Init();
	ret = RK_MPI_AO_SetChnAttr(AO_CHN_NUM, &g_ao_attr);
	ret |= RK_MPI_AO_EnableChn(AO_CHN_NUM);
	if (ret) {
		printf("ERROR: create ao[%d] failed! ret=%d\n", AO_CHN_NUM, ret);
		return -1;
	}

	ret = RK_MPI_AO_GetVolume(AO_CHN_NUM, &s32CurrentVolmue);
	if (ret) {
		printf("Get Volume(before) failed! ret=%d\n", ret);
		return -1;
	}
	printf("#Before Volume set, volume=%d\n", s32CurrentVolmue);

	ret = RK_MPI_AO_SetVolume(AO_CHN_NUM, vol);
	if (ret) {
		printf("Set Volume failed! ret=%d\n", ret);
		return -1;
	}

	s32CurrentVolmue = 0;
	ret = RK_MPI_AO_GetVolume(AO_CHN_NUM, &s32CurrentVolmue);
	if (ret) {
		printf("Get Volume(after) failed! ret=%d\n", ret);
		return -1;
	}
	printf("#After Volume set, volume=%d\n", s32CurrentVolmue);

	printf("%s initial finish\n", __func__);

	return 0;
}

int ao_exit(void)
{
	printf("%s exit!\n", __func__);
	return RK_MPI_AO_DisableChn(AO_CHN_NUM);
}

int ao_start(char *path)
{
	if(g_play_running){
		printf("g_play_thread_id is running.\n");
		return -1;
	}
	else{
		pthread_create(&g_play_thread_id, NULL, play_thread, path);
		return 0;
	}
}

int ao_stop(void)
{
	g_play_running = 0;
	return pthread_join(g_play_thread_id, NULL);
}

int ai_init(SAMPLE_FORMAT_E fmt, int rate, int chn, int vol)
{
	int ret = 0;
	RK_S32 s32CurrentVolmue = 0;

	RK_MPI_SYS_Init();

	g_ai_attr.pcAudioNode = "default";
	g_ai_attr.enSampleFormat = fmt;
	g_ai_attr.u32NbSamples = 1024;
	g_ai_attr.u32SampleRate = rate;
	g_ai_attr.u32Channels = chn;
	g_ai_attr.enAiLayout = AI_LAYOUT_NORMAL;

	ret = RK_MPI_AI_SetChnAttr(AI_CHN_NUM, &g_ai_attr);
	ret |= RK_MPI_AI_EnableChn(AI_CHN_NUM);
	if (ret) {
		printf("Enable AI[0] failed! ret=%d\n", ret);
		return -1;
	}

	ret = RK_MPI_AI_GetVolume(AI_CHN_NUM, &s32CurrentVolmue);
	if (ret) {
		printf("Get Volume(before) failed! ret=%d\n", ret);
		return -1;
	}
	printf("#Before Volume set, volume=%d\n", s32CurrentVolmue);
	
	ret = RK_MPI_AI_SetVolume(AI_CHN_NUM, vol);
	if (ret) {
		printf("Set Volume failed! ret=%d\n", ret);
		return -1;
	}

	s32CurrentVolmue = 0;
	ret = RK_MPI_AI_GetVolume(AI_CHN_NUM, &s32CurrentVolmue);
	if (ret) {
		printf("Get Volume(after) failed! ret=%d\n", ret);
		return -1;
	}
	printf("#After Volume set, volume=%d\n", s32CurrentVolmue);

	return 0;
}

int ai_start(char *path)
{
	int ret = 0;

	if(g_rec_running){
		printf("g_rec_thread_id is running.\n");
		return -1;
	}
	else{
		pthread_create(&g_rec_thread_id, NULL, rec_thread, path);
		ret = RK_MPI_AI_StartStream(AI_CHN_NUM);
		if (ret) {
			printf("Start AI failed! ret=%d\n", ret);
			return -1;
		}
	}

	return 0;
}

int ai_exit(void)
{
	return RK_MPI_AI_DisableChn(AI_CHN_NUM);
}

int ai_stop(void)
{
	g_rec_running = 0;
	return pthread_join(g_rec_thread_id, NULL);
}

