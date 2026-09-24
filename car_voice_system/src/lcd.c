#include "lcd.h"
#include <stdlib.h>

int lcd_fd;
unsigned char *lcd = NULL;

/* lcd 初始化：打开 /dev/fb0，映射 800x480x4 帧缓冲 */
int lcd_init(void)
{
    lcd_fd = open("/dev/fb0", O_RDWR);
    if (lcd_fd < 0)
    {
        perror("open /dev/fb0 failed");
        return -1;
    }

    lcd = mmap(NULL, 800*480*4, PROT_READ|PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if (lcd == MAP_FAILED)
    {
        perror("mmap /dev/fb0 failed");
        close(lcd_fd);
        return -1;
    }
    return 0;
}

/* 显示 24 位 BMP 图片到 (x0, y0)。BMP 像素数据 BGR 顺序，LCD 帧缓冲 BGRA */
int show_bmp(char *bmpfile, int x0, int y0)
{
    int bmp_fd = open(bmpfile, O_RDWR);
    if (bmp_fd < 0)
    {
        perror("open bmp failed");
        return -1;
    }

    int w = 0, h = 0;
    lseek(bmp_fd, 18, SEEK_SET);
    if (read(bmp_fd, &w, 4) != 4 || read(bmp_fd, &h, 4) != 4) {
        close(bmp_fd);
        return -1;
    }

    /* 边界检查：避免 w=0 / h=0 / 过大导致 malloc 失败或越界 */
    if (w <= 0 || h <= 0 || w > 2048 || h > 2048) {
        fprintf(stderr, "bmp size invalid: w=%d h=%d\n", w, h);
        close(bmp_fd);
        return -1;
    }

    printf("w %d h %d\n", w, h);
    lseek(bmp_fd, 54, SEEK_SET);

    unsigned char *bmp = (unsigned char *)malloc(w * h * 3);
    if (!bmp) {
        perror("malloc bmp failed");
        close(bmp_fd);
        return -1;
    }
    if (read(bmp_fd, bmp, w * h * 3) != w * h * 3) {
        fprintf(stderr, "bmp read incomplete\n");
        free(bmp);
        close(bmp_fd);
        return -1;
    }

    int x, y;
    for (y = y0; y < h + y0; y++)
    {
        if (y < 0 || y >= 480) continue;
        for (x = x0; x < w + x0; x++)
        {
            if (x < 0 || x >= 800) continue;
            lcd[y*800*4 + x*4 + 0] = bmp[(h-1-(y-y0))*w*3 + (x-x0)*3 + 0]; /* B */
            lcd[y*800*4 + x*4 + 1] = bmp[(h-1-(y-y0))*w*3 + (x-x0)*3 + 1]; /* G */
            lcd[y*800*4 + x*4 + 2] = bmp[(h-1-(y-y0))*w*3 + (x-x0)*3 + 2]; /* R */
            lcd[y*800*4 + x*4 + 3] = 0;                                    /* A */
        }
    }

    free(bmp);
    close(bmp_fd);
    return 0;
}

/* 关闭 lcd */
int lcd_close(void)
{
    close(lcd_fd);
    munmap(lcd, 800*480*4);
    return 0;
}