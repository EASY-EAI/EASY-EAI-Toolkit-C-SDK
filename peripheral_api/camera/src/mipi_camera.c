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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "aiq_control.h"
#include <rga/RgaApi.h>
#include <rkaiq/rkisp_api.h>

/* 宏定义 */
#define	DEFAULT_ISP_FMT		RK_FORMAT_YCbCr_420_SP	//执行control_init时默认fmt W H
#define	CAM_WIDTH			1280
#define	CAM_HEIGHT			720
#define	BUF_NUM				3
#define	DEFAULT_OUTPUT_FMT	RK_FORMAT_RGB_888

typedef struct {
	int cam_type;
	int width;
	int height;
	int fmt;
	int rotation;
	const struct rkisp_api_ctx *ctx;
}mipi_camera;

/* 全局变量 */
static mipi_camera rgb;
static mipi_camera ir;
static	int aiq_flag = 0;	//双mipi摄像头只要执行一次aiq_control_alloc(),用此变量控制执行次数

static int camrgb_control_init(void)
{
    rgb.ctx = rkisp_open_device2(rgb.cam_type);

    if (rgb.ctx == NULL) {
        printf("%s: ctx is NULL\n", __func__);
        return -1;
    }

    rkisp_set_buf(rgb.ctx, BUF_NUM, NULL, 0);
    rkisp_set_fmt(rgb.ctx, CAM_WIDTH, CAM_HEIGHT, V4L2_PIX_FMT_NV12);

    if (rkisp_start_capture(rgb.ctx))
        return -1;

    return 0;
}

static int camir_control_init(void)
{
    ir.ctx = rkisp_open_device2(ir.cam_type);

    if (ir.ctx == NULL) {
        printf("%s: ctx is NULL\n", __func__);
        return -1;
    }

    rkisp_set_buf(ir.ctx, BUF_NUM, NULL, 0);
    rkisp_set_fmt(ir.ctx, CAM_WIDTH, CAM_HEIGHT, V4L2_PIX_FMT_NV12);

    if (rkisp_start_capture(ir.ctx))
        return -1;

    return 0;
}

static int mipicamera_getframe(mipi_camera *cam, char *pbuf)
{
	int width = 0;
    int height = 0;
	RgaSURF_FORMAT fmt = 0;
	int rot = 0;
	rga_info_t src, dst;
	int ret = -1;
	const struct rkisp_api_buf *buf;

	if (!cam || !pbuf) {
		printf("%s: NULL PTR!\n", __func__);
		return -1;
	}

	width = cam->width;
	height = cam->height;
	fmt = cam->fmt;
	rot = cam->rotation;

	buf = rkisp_get_frame(cam->ctx, 0);
	if (!buf) {
		printf("%s: NULL PTR! Line:%d\n", __func__, __LINE__);
		return -1;
	}

	//图像参数转换
	memset(&src, 0, sizeof(rga_info_t));
	src.fd = -1;
	src.virAddr = buf->buf;
	src.mmuFlag = 1;
	src.rotation = rot;
	rga_set_rect(&src.rect, 0, 0, CAM_WIDTH, CAM_HEIGHT, CAM_WIDTH, CAM_HEIGHT, DEFAULT_ISP_FMT);

	memset(&dst, 0, sizeof(rga_info_t));
	dst.fd = -1;
	dst.virAddr = pbuf;
	dst.mmuFlag = 1;
	rga_set_rect(&dst.rect, 0, 0, width, height, width, height, fmt);
	if (c_RkRgaBlit(&src, &dst, NULL)) {
		printf("%s: rga fail\n", __func__);
		ret = -1;
	}
	else {
		ret = 0;
	}

	rkisp_put_frame(cam->ctx, buf);
	return ret;
}

static int mipicamera_init(mipi_camera *cam)
{
	int cam_type = 0;

	if (!cam) {
		printf("%s: NULL PTR cam\n", __func__);
		return -1;
	}
	cam_type = cam->cam_type;

	if (!aiq_flag) {
		aiq_flag++;
		if (c_RkRgaInit()) {
        	printf("%s: rga init fail!\n", __func__);
		}
		aiq_control_alloc();
	}

	if (cam->cam_type == CAM_TYPE_RKISP1) {
		for (int i = 0; i < 10; i++) {
        if (aiq_control_get_status(AIQ_CONTROL_RGB)) {
            	printf("%s: RGB aiq status ok.\n", __func__);
            	camrgb_control_init();
            	break;
        	}
        	sleep(1);
    	}
	}

	if (cam->cam_type == CAM_TYPE_RKCIF) {
		for (int i = 0; i < 10; i++) {
        if (aiq_control_get_status(AIQ_CONTROL_IR)) {
            	printf("%s: RGB aiq status ok.\n", __func__);
            	camir_control_init();
            	break;
        	}
        	sleep(1);
    	}
	}

	return 0;
}

static void mipicamera_exit(mipi_camera *cam)
{
	if (!cam) {
		printf("%s: NULL PTR cam.\n", __func__);
		return;
	}

	rkisp_stop_capture(cam->ctx);
    rkisp_close_device(cam->ctx);
	usleep(500000); //TODO
}

int rgbcamera_init(int width, int height, int rot) {
	rgb.cam_type = CAM_TYPE_RKISP1;
	rgb.width = width;
	rgb.height = height;
	rgb.fmt = DEFAULT_OUTPUT_FMT;
	switch(rot) {
	case 90:
		rgb.rotation = HAL_TRANSFORM_ROT_90;
		break;
	case 180:
		rgb.rotation = HAL_TRANSFORM_ROT_180;
		break;
	case 270:
		rgb.rotation = HAL_TRANSFORM_ROT_270;
		break;
	default:
		rgb.rotation = 0;
		break;
	}
	return mipicamera_init(&rgb);
}

void rgbcamera_exit(void)
{
	mipicamera_exit(&rgb);
}

int rgbcamera_getframe(char *pbuf)
{
	return mipicamera_getframe(&rgb, pbuf);
}

int ircamera_init(int width, int height, int rot) {
	ir.cam_type = CAM_TYPE_RKCIF;
	ir.width = width;
	ir.height = height;
	ir.fmt = DEFAULT_OUTPUT_FMT;
	switch(rot) {
	case 90:
		ir.rotation = HAL_TRANSFORM_ROT_90;
		break;
	case 180:
		ir.rotation = HAL_TRANSFORM_ROT_180;
		break;
	case 270:
		ir.rotation = HAL_TRANSFORM_ROT_270;
		break;
	default:
		ir.rotation = 0;
		break;
	}
	return mipicamera_init(&ir);
}

void ircamera_exit(void)
{
	mipicamera_exit(&ir);
}

int ircamera_getframe(char *pbuf)
{
	return mipicamera_getframe(&ir, pbuf);
}

