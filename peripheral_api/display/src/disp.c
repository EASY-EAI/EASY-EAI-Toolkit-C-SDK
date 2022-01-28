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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <rga/RgaApi.h>
#include "lmodrm_display.h"

#define	BUF_COUNT			3
#define	DEFAULT_INPUT_FMT	RK_FORMAT_RGB_888
#define	DEFAULT_DISPLAY_FMT	RK_FORMAT_YCbCr_420_SP

struct display {
    int fmt; //刷到硬件的fmt只能是nv12, 采用drm风格
    RgaSURF_FORMAT input_fmt; //采用rga风格
    int width;
    int height;
    int plane_type;
    struct drm_dev dev;
    struct drm_buf buf[BUF_COUNT];
};

struct display g_disp;

static int drm_display_init(struct display *disp, int pz, int oz)
{
    int ret;
    if (lmo_drmInit(&disp->dev, pz, oz)) {
        fprintf(stderr, "drmInit: Failed\n");
        return -1;
    }

    for (int i = 0; i < BUF_COUNT; i++) {
        ret = lmo_drmGetBuffer(disp->dev.drm_fd, disp->width, disp->height, disp->fmt, &disp->buf[i]);
        if (ret) {
            fprintf(stderr, "Alloc drm buffer failed, %d\n", i);
            return -1;
        }
    }

    return 0;
}

int disp_init(int width, int height)
{
    int ret;

	if (c_RkRgaInit())
        printf("%s: rga init fail!\n", __func__);

    g_disp.fmt = DRM_FORMAT_NV12;
	g_disp.input_fmt = DEFAULT_INPUT_FMT;
    g_disp.width = width;
    g_disp.height = height;
    g_disp.plane_type = DRM_PLANE_TYPE_OVERLAY;
#ifdef QT_MAJOR_VERSION
    ret = drm_display_init(&g_disp, 1, 0); //DRM_PLANE_TYPE_PRIMARY on top
#else
	ret = drm_display_init(&g_disp, 0, 1); //DRM_PLANE_TYPE_OVERLAY on top
#endif
    if (ret)
        return ret;

    return 0;
}

void disp_set_format(RgaSURF_FORMAT fmt)
{
	g_disp.input_fmt = fmt;
}

static void drm_display_exit(struct display *disp)
{
    lmo_drmDeinit(&disp->dev);
    for (int i = 0; i < BUF_COUNT; i++)
        lmo_drmPutBuffer(disp->dev.drm_fd, &disp->buf[i]);
}

void disp_exit(void)
{
    drm_display_exit(&g_disp);
}

static void drm_commit(struct display *disp, int num, void *ptr, int x_off, int y_off)
{
    int ret = 0;
	rga_info_t src, dst;
    char *map = disp->buf[num].map;

	memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1;
    src.virAddr = ptr;
    src.mmuFlag = 1;
    //src.rotation = 0;
    rga_set_rect(&src.rect, 0, 0, disp->width, disp->height, disp->width, disp->height, disp->input_fmt);
    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = -1;
    dst.virAddr = map;
    dst.mmuFlag = 1;
    rga_set_rect(&dst.rect, 0, 0, disp->width, disp->height, disp->width, disp->height, DEFAULT_DISPLAY_FMT);
    if (c_RkRgaBlit(&src, &dst, NULL)) {
        printf("%s: rga fail\n", __func__);
        return;
    }

    //memcpy(map, ptr, disp->width * disp->height * 1.5);
    ret = lmo_drmCommit(&disp->buf[num], disp->width, disp->height, x_off, y_off, &disp->dev, disp->plane_type);
    if (ret) {
        fprintf(stderr, "display commit error, ret = %d\n", ret);
    }
}

void disp_commit(void *ptr, int x_off, int y_off)
{
    static int num = 0;

    drm_commit(&g_disp, num, ptr, x_off, y_off);
    num = (num + 1) % BUF_COUNT;
}

