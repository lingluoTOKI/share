#ifndef  TS_H_
#define  TS_H_
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
int ts_init();
void ts_close();
//int get_xy();
void get_xy(int *tx, int *ty);
#endif