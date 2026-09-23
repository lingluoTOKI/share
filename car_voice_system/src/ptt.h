#ifndef __PTT_H__
#define __PTT_H__

#include "common.h"

/* ================ "按住说话"按钮区域 ================
 * 位于屏幕中下部，进入语音阶段后显示；不与底部对话条(420起)重叠
 */
#define PTT_X0      250
#define PTT_Y0      330
#define PTT_X1      550
#define PTT_Y1      405

/* 最长录音秒数：超时自动结束，防止一直按住 */
#define PTT_MAX_SEC 10

/* 太短的录音（帧数）视为无效，16000帧=1秒 */
#define PTT_MIN_FRAMES 8000

/* ---------------- 录音控制（内部用录音线程） ---------------- */
int  recorder_start(char *pcmfile);   /* 开始录音到指定裸PCM文件 */
void recorder_stop(void);            /* 停止录音并关闭文件 */
long recorder_frames(void);          /* 已录制的采样帧数 */

/* ---------------- 触摸按下/松开检测 ---------------- */
int  touch_wait_press(int x0, int y0, int x1, int y1);  /* 等待在区域内按下 */
int  touch_wait_release(int timeout_sec);  /* 等待松开：1=已松开 0=超时 */

/* ---------------- 绘制"按住说话"按钮 ---------------- */
void draw_ptt_button(font *ft, char *label);

#endif
