#ifndef __CAR_VOICE_H__
#define __CAR_VOICE_H__

#include "common.h"
#include "font.h"
#include "lcd.h"
#include "ts.h"

/* ================= 屏幕与布局常量 ================= */
#define SCREEN_W    800
#define SCREEN_H    480

/* 标题栏（顶部）：标题 + 状态 */
#define TITLE_Y     0
#define TITLE_H     56

/* 场景显示区（中间） */
#define SCENE_X     0
#define SCENE_Y     56
#define SCENE_W     800
#define SCENE_H     324

/* 对话条（底部偏上） */
#define DIALOG_X    0
#define DIALOG_Y    380
#define DIALOG_W    800
#define DIALOG_H    40

/* 底部按钮（"按回车说话"） */
#define BTN_X0      0
#define BTN_Y0      420
#define BTN_X1      800
#define BTN_Y1      480

/* 启动界面【启动】按钮（绘制 + 触摸判定使用同一矩形） */
#define START_BTN_X0  320
#define START_BTN_Y0  310
#define START_BTN_X1  480
#define START_BTN_Y1  380

/* 触摸屏原始坐标范围 */
#define TS_RAW_MAX_X   800
#define TS_RAW_MAX_Y   480

/* ================= 语音指令 ID 映射 ================= */
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

/* ================= LED 硬件灯号 ================= */
#define LED_LIGHT_L    7
#define LED_LIGHT_R    8

/* ================= 外部全局 LCD 指针 ================= */
extern unsigned char *lcd;

/* ================= 业务函数 ================= */
int  led_ctl(int n, int sta);
void show_start_screen(font *ft);
void show_main_screen(font *ft);
void show_hello_screen(font *ft);
void show_bye_screen(font *ft);
void show_led_scene(font *ft, int on);
void show_ac_scene(font *ft, int temp, int on);
void show_win_scene(font *ft, int open);
void show_dialog(font *ft, char *text);
void fill_screen_color(int r, int g, int b);
void print_greeting(font *ft);
void draw_talk_button(font *ft);
void draw_start_button(font *ft);
void show_status(font *ft);
int  wait_talk_button(void);
int  wait_start_button(void);

#endif