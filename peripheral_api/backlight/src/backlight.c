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
#include <stdint.h>
#include <string.h>

#define	BRIGHTNESS	"/sys/class/backlight/backlight/brightness"

int set_backlight(char *path, uint16_t level)
{
	FILE *fp = NULL;
	int ret = 0;
	char cmd[16] = {0};

	if (path) {
		fp = fopen(path, "r+");
		printf("setting brightness with: %s\n", path);
	}
	else {
		fp = fopen("BRIGHTNESS", "r+");
		printf("setting brightness with: %s\n", path);
	}
	if (!fp) {
		printf("%s: %d ERROR!\n", __func__, __LINE__);
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "%d", level);
	fseek(fp, 0L, SEEK_SET);
	ret = fwrite(cmd, 1, strlen(cmd) + 1, fp);

	fclose(fp);

	if (ret > 0) {
		return 0;
	}
	else {
		return -1;
	}
}
