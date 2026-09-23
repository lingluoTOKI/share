#include "ts.h"
#include "lcd.h"
#include <stdio.h>
int main(void)
{
    int tx, ty;
    ts_init();          //触摸屏初始化，打开/dev/input/event0
    lcd_init();         //LCD初始化，mmap映射/dev/fb0显存
    show_bmp("1.bmp", 0, 0); //全屏显示800*480、24位bmp图片

    while(1)
    {
        get_xy(&tx, &ty);   //阻塞等待手指抬起，返回换算后LCD像素坐标
        printf("触摸坐标 x=%d, y=%d\n", tx, ty);

        //实际测试校准后的按钮矩形区域
        if(tx>280 && tx<330 && ty>300 && ty<335)
        {
            printf("====点击暂停按钮====\n");
            break;
        }
    }

    lcd_close();    //关闭LCD，解除mmap映射
    ts_close();     //关闭触摸屏设备
    return 0;
}
