#include "font.h"
#include "lcd.h"
#include <stdio.h>

int main(void)
{
    lcd_init();

    font *ft = fontLoad("simfang.ttf");
    if (ft == NULL)
    {
        printf("font load error\n");
        lcd_close();
        return -1;
    }
    printf("font load ok\n");

    fontSetSize(ft, 64);

    // 浅暖橙色背景（不透明，视觉柔和不刺眼）
    bitmap *bm = createBitmapWithInit(300, 100, 4, getColor(255, 100, 200, 255));

    // 黑色文字，画布内居中
    fontPrint(ft, bm, 42, 20, "欧阳宸", getColor(255, 0, 0, 0), 300);

    // 整体拷贝到屏幕居中位置
    int x, y;
    for (y = 0; y < bm->height; y++)
    {
        for (x = 0; x < bm->width; x++)
        {
            lcd[(190 + y) * 800 * 4 + (250 + x) * 4 + 0] = bm->map[y * bm->width * 4 + x * 4 + 0];
            lcd[(190 + y) * 800 * 4 + (250 + x) * 4 + 1] = bm->map[y * bm->width * 4 + x * 4 + 1];
            lcd[(190 + y) * 800 * 4 + (250 + x) * 4 + 2] = bm->map[y * bm->width * 4 + x * 4 + 2];
            lcd[(190 + y) * 800 * 4 + (250 + x) * 4 + 3] = bm->map[y * bm->width * 4 + x * 4 + 3];
        }
    }

    destroyBitmap(bm);
    fontUnload(ft);
    lcd_close();
    return 0;
}
