#ifndef  LCD_H_
#define  LCD_H_
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include <sys/mman.h>
int lcd_init();
int show_bmp(char *bmpfile, int x0, int y0);
int lcd_close();

#endif