#include "lcd.h"
int lcd_fd;
unsigned char *lcd = NULL;
//lcd初始化
void lcd_init()
{
    //1. 打开lcd驱动（“/dev/fb0”）  open
    lcd_fd = open("/dev/fb0",O_RDWR);
	
	//给驱动申请虚拟共享内存
	lcd = mmap(NULL,800*480*4,PROT_READ|PROT_WRITE,MAP_SHARED,lcd_fd,0);
	
	
}
//显示图片
void show_bmp(char *bmpfile, int x0, int y0)//2.bmp  400*240
{
	//打开图片
	int bmp_fd = open(bmpfile,O_RDWR);
	
	//读取属性数据--w  h
	int  w=0, h=0;
       
	//把偏移位置设置到18
	lseek(bmp_fd, 18, SEEK_SET);
	
	//读取图片宽度
	read(bmp_fd,&w,4);
	//读取图片高度
	read(bmp_fd,&h,4);	
	printf("w %d h %d\n",w,h);
		
	//处理头54个字节属性数据
	lseek(bmp_fd,54,SEEK_SET);
	//read(bmp_fd,buf,54);
	//读取颜色数据
	unsigned char bmp[w*h*3];
	read(bmp_fd,bmp,w*h*3);
	
    //2.准备颜色数据放入虚拟内存
   // unsigned char lcd[800*480*4]={0};
    //指针和数组之间，在应用时形式上可以相互转换 
	int x,y;
	for(y=y0; y<h+y0; y++)
	{
		for(x=x0;x<w+x0; x++)
		{
			lcd[y*800*4+x*4+0] = bmp[(h-1-(y-y0))*w*3+(x-x0)*3+0];//B
	        lcd[y*800*4+x*4+1] = bmp[(h-1-(y-y0))*w*3+(x-x0)*3+1];//G
	        lcd[y*800*4+x*4+2] = bmp[(h-1-(y-y0))*w*3+(x-x0)*3+2];//R
	        lcd[y*800*4+x*4+3] = 0;//A			
		}
		
	}

    //4.关闭文件close
	close(bmp_fd);

}
//关闭lcd
void lcd_close()
{
	//4.关闭文件close
    close(lcd_fd);
    //释放内存
	munmap(lcd,800*480*4);
}