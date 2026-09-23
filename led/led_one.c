#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
int fd,ret;	
//打开led驱动
int led_drv_open()
{	
	//打开led驱动
	fd = open("/dev/led_drv", O_RDWR);
	if(fd < 0)//判断是否出现问题，是否成功打开led驱动
	{
		perror("open led_drv");
		return -1;		
	}	
}	
//led状态控制
//n:灯号 
//sta: 状态  1 开灯  0关灯
int led_ctl(int n, int sta)
{
	char led_ctrl[2];
	   
	//设置控制参数
	led_ctrl[1] = n; // 灯号 D9   7   8   9  10 
	led_ctrl[0] = sta; // 状态 1开灯	0关灯	
	ret = write(fd, led_ctrl, sizeof(led_ctrl));
	if(ret != 2)
	{
		perror("write");
	}
	
}		
int main(void)
{
	//打开led驱动
    led_drv_open();
	
	//led状态控制--关闭所有led
    led_ctl(7,0);
	led_ctl(8,0);
	led_ctl(9,0);	
	led_ctl(10,0);
	
	//输入数据控制led
	int n,sta;
	
	printf("灯号8 9 10\n");
	scanf("%d",&n);
	
	printf("状态 1开灯  0关灯\n");
	scanf("%d",&sta);
	
	led_ctl(n, sta);
	
	
	
	
	//关闭驱动
	close(fd);
	return 0;
}