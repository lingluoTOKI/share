#include "lcd.h"


int main()
{
	//lcd初始化
	lcd_init();
	
	//显示bmp图片
	show_bmp("1.bmp", 0,0);
	
	
	//关闭lcd
	lcd_close();
	
}