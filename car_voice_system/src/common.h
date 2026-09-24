#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <termios.h>
#include <pthread.h>
#include <jpeglib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/* 语音识别引擎端口，由 Ubuntu 侧 ./main 服务器监听 */
#define DEF_PORT    54321
#define XMLSIZE     4096
#define XMLFILE     "./result.xml"

/* 显示屏相关（common.c 中 jpeg 显示使用） */
#define WIDTH       800
#define HEIGHT      480
#define BLIND       8

typedef unsigned long (*stride)[WIDTH];

struct argument {
    stride FB;
    stride image;
    int offset;
    int flag;
};

enum { IN = 1, OUT = 0 };

/* 通用函数 */
int Open(const char *pathname, int flag);
ssize_t Write(int fildes, const void *buf, size_t nbyte);
ssize_t Read(int fildes, void *buf, size_t nbyte);
int init_sock(const char *ubuntu_ip);
void send_pcm(int sockfd, char *pcmfile);
xmlChar *wait4id(int sockfd);
void wait4touch(void);
void wait_key_input(void);
xmlChar *parse_xml(char *xmlfile);
char *init_lcd(void);
void show_someone(char *FB, int id);
void show_in(char *FB, char *filename);
void fade_out(char *FB, char *filename);
char *get_rgb(char *filename);
void blind_window_in(char *FB, unsigned long (*image)[WIDTH]);
void blind_window_out(char *FB, unsigned long (*image)[WIDTH]);
void blind_window(char *FB, unsigned long (*image)[WIDTH]);

#endif