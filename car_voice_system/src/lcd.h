#ifndef __LCD_H__
#define __LCD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int lcd_init(void);
int show_bmp(char *bmpfile, int x0, int y0);
int lcd_close(void);

#endif