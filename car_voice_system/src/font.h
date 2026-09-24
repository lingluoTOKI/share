#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef int16_t  s16;
typedef u32 color;

typedef struct {
    u32 width;
    u32 height;
    u32 byteperpixel;
    u8 *map;
} bitmap;

/* stb_truetype ��������Ϣ�ṹ��ǰ������Ϊ���������ͣ� */
typedef struct stbtt_fontinfo stbtt_fontinfo;

typedef struct {
    stbtt_fontinfo *info;
    u8 *buffer;
    float scale;
} font;

#define getColor(a, b, c, d) (a|b<<8|c<<16|d<<24)

bitmap *createBitmap(u32 width, u32 height, u32 byteperpixel);
void destroyBitmap(bitmap *bm);
color getPixel(bitmap *bm, u32 x, u32 y);
void setPixel(bitmap *bm, u32 x, u32 y, color c);
bitmap *createBitmapWithInit(u32 width, u32 height, u32 byteperpixel, color c);
void fontPrint(font *f, bitmap *screen, s32 x, s32 y, char *text, color c, s32 maxWidth);
void fontSetSize(font *f, s32 pixels);
font *fontLoad(char *fontPath);
void fontUnload(font *f);

#endif