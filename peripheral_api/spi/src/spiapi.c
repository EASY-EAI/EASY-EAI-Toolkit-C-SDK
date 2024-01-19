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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include "clist.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
	int bus;
	int select;
	int fd;
	uint32_t mode;
	uint8_t bits;
	uint32_t speed;
	uint16_t delay;
}spidev_t;

static CList *g_dev_list;


static int do_spi_transfer(int fd, uint32_t mode, uint16_t delay, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		//.speed_hz = speed,
		//.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

	return ret;
}

int spi_transfer(int bus, int select, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int dev_num = 0;
	spidev_t *tmp = NULL;

	if (!g_dev_list) {
		return -1;
	}

	dev_num = g_dev_list->count(g_dev_list);
	for (int i = 0; i < dev_num; i++) {
		tmp = (spidev_t *)g_dev_list->at(g_dev_list, i);
		if (tmp && tmp->bus == bus && tmp->select == select) {
			return do_spi_transfer(tmp->fd, tmp->mode, tmp->delay, tx, rx, len);
		}
	}
	return -1;
}

static spidev_t *do_spi_init(int bus, int select, uint32_t mode, uint8_t bits, uint32_t speed, uint16_t delay)
{
	spidev_t *dev = NULL;
	char dev_node[16] = {0};
	int ret = 0;

	dev = (spidev_t *)malloc(sizeof(spidev_t));
	if (!dev) {
		goto error1;
	}

	memset(dev, 0, sizeof(spidev_t));
	dev->bus = bus;
	dev->select = select;
	dev->mode = mode;
	dev->bits = bits;
	dev->speed = speed;
	dev->delay = delay;

	/* open */
	snprintf(dev_node, sizeof(dev_node), "/dev/spidev%d.%d", bus, select);
	dev->fd = open(dev_node, O_RDWR);
	if (dev->fd < 0) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error2;
	}

	/* spi mode */
	ret = ioctl(dev->fd, SPI_IOC_WR_MODE32, &mode);
   	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	ret = ioctl(dev->fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	/* bit per word */
	ret = ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	ret = ioctl(dev->fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	/* max speed hz */
	ret = ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	ret = ioctl(dev->fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
 	if (ret == -1) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		goto error3;
	}

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	return dev;

error3:
	close(dev->fd);
error2:
	free(dev);
error1:
	return NULL;
}

static void do_spi_exit(spidev_t *dev)
{
	if (!dev) {
		return;
	}

	printf("closing spidev at bus%d : select%d.\n", dev->bus, dev->select);
    close(dev->fd);
}

int spi_init(int bus, int select, uint32_t mode, uint8_t bits, uint32_t speed, uint16_t delay)
{
	int dev_num = 0;
	spidev_t *tmp = NULL;

	if (!g_dev_list) {
		g_dev_list = CList_Init(sizeof(spidev_t));
		if (!g_dev_list)
			return -1;
	}

	dev_num = g_dev_list->count(g_dev_list);
	for (int i = 0; i < dev_num; i++) {
		tmp = (spidev_t *)g_dev_list->at(g_dev_list, i);
		if (tmp && tmp->bus == bus && tmp->select == select) {
			printf("spidev at bus %d : select %d had init.\n", bus, select);
			return -1;
		}
	}

	tmp = do_spi_init(bus, select, mode, bits, speed, delay);
	if (!tmp) {
		printf("%s: %d exit!\n", __func__, __LINE__);
		return -1;
	}
	g_dev_list->add(g_dev_list, tmp);

    return 0;
}

void spi_exit(int bus, int select)
{
	int dev_num = 0;
	spidev_t *tmp = NULL;

	if (!g_dev_list) {
		return;
	}

	dev_num = g_dev_list->count(g_dev_list);
	for (int i = 0; i < dev_num; i++) {
		tmp = (spidev_t *)g_dev_list->at(g_dev_list, i);
		if (tmp && tmp->bus == bus && tmp->select == select) {
			do_spi_exit(tmp);
			g_dev_list->remove(g_dev_list, i);
			break;
		}
	}

	dev_num = g_dev_list->count(g_dev_list);
	if(dev_num == 0) {
		g_dev_list->free(g_dev_list);
		g_dev_list = NULL;
	}
}
