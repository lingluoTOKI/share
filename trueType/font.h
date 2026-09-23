#ifndef __font_h__
#define __font_h__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#define color u32
#define getColor(a, b, c, d) (a|b<<8|c<<16|d<<24)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef struct stbtt_fontinfo
{
   void           * userdata;
   unsigned char  * data;
   int              fontstart;
   int numGlyphs;
   int loca,head,glyf,hhea,hmtx,kern;
   int index_map;
   int indexToLocFormat;
} stbtt_fontinfo;

typedef struct{
    u32 height;
    u32 width;
    u32 byteperpixel;
    u8 *map;
}bitmap;

typedef struct{
    stbtt_fontinfo *info;
    u8 *buffer;
    float scale;
}font;

// 字库相关
font *fontLoad(char *fontPath);
void fontSetSize(font *f, s32 pixels);
bitmap *createBitmap(u32 width, u32 height, u32 byteperpixel);
bitmap *createBitmapWithInit(u32 width, u32 height, u32 byteperpixel, color c);
void fontPrint(font *f, bitmap *screen, s32 x, s32 y, char *text, color c, s32 maxWidth);
void destroyBitmap(bitmap *bm);
void fontUnload(font *f);

#endif