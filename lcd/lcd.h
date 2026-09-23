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
void lcd_init();
void show_bmp(char *bmpfile, int x0, int y0);
void lcd_close();

#endif