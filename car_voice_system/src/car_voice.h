#ifndef __CAR_VOICE_H__
#define __CAR_VOICE_H__

#include "common.h"
#include "font.h"
#include "lcd.h"
#include "ts.h"

/* ================= 屏幕与布局常量 ================= */
#define SCREEN_W    800
#define SCREEN_H    480

/* 对话展示区：屏幕底部文字条 */
#define DIALOG_X    40
#define DIALOG_Y    420
#define DIALOG_W    720
#define DIALOG_H    48

/* 触摸启动按钮区域（主界面上的"启动语音控制"按钮） */
#define BTN_X0      280
#define BTN_Y0      355
#define BTN_X1      520
#define BTN_Y1      410

/* ================= 语音指令 ID 映射 =================
 * 对应 Ubuntu 端 cmd.bnf 语法中为每条指令分配的 id
 * 1   : 你好 / 唤醒
 * 2   : 再见 / 退出
 * 10  : 打开车灯
 * 11  : 关闭车灯
 * 20  : 打开空调
 * 21  : 关闭空调
 * 22  : 空调升温
 * 23  : 空调降温
 * 30  : 打开车窗
 * 31  : 关闭车窗
 * 100 : 显示当前场景
 * =================================================== */
#define CMD_HELLO       1
#define CMD_BYE         2
#define CMD_LED_ON     10
#define CMD_LED_OFF    11
#define CMD_AC_ON      20
#define CMD_AC_OFF     21
#define CMD_AC_HOT     22
#define CMD_AC_COOL    23
#define CMD_WIN_OPEN   30
#define CMD_WIN_CLOSE  31
#define CMD_SCENE      100

/* ================= LED 硬件灯号 =================
 * GEC6818 板载 LED：7/8/9/10 号，用于模拟车载灯光
 * ================================================= */
#define LED_LIGHT_L    7   /* 左前大灯 */
#define LED_LIGHT_R    8   /* 右前大灯 */

/* ================= 外部全局 LCD 指针 ================= */
extern unsigned char *lcd;

/* ================= 业务函数 ================= */
int  led_ctl(int n, int sta);                      /* LED 控制 */
void show_start_screen(font *ft);                  /* 启动界面 */
void show_main_screen(font *ft);                   /* 语音控制主界面 */
void show_led_scene(font *ft, int on);             /* 车灯场景 */
void show_ac_scene(font *ft, int temp, int on);    /* 空调场景 */
void show_win_scene(font *ft, int open);           /* 车窗场景 */
void show_dialog(font *ft, char *text);            /* 对话展示区 */
void fill_screen_color(int r, int g, int b);       /* 整屏填充 */
int  wait_touch_button(void);                      /* 等待触摸启动按钮 */
void print_greeting(font *ft);                     /* 开机问候语 */

#endif
