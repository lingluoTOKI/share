#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>

int tx,ty;
int ts_fd;

//触摸屏初始化
int ts_init()
{
	 //打开触摸驱动
	ts_fd = open("/dev/input/event0",O_RDWR);
	if(ts_fd < 0)
	{
		perror("open ts faile\n");
		return -1;
	}
	return 0;
}

//关闭触摸屏
int ts_close()
{
	close(ts_fd);
	return 0;
}

//获取一次触摸坐标（松手时返回，返回0表示成功）
int get_xy(int *tx, int *ty)
{
	int x = 0, y = 0;

	//定义一个输入子系统模型结构体
	struct input_event ts;

	//读取触摸数据--》输入子系统模型
	while(1)
	{
	    read(ts_fd, &ts, sizeof(ts));//阻塞等待触摸动作产生---按下
	    if(ts.type == EV_ABS)
	    {
	    	if(ts.code == ABS_X)
	    	{
	    		x = ts.value ;
	    	}
	    	if(ts.code == ABS_Y)
	    	{
	    		y = ts.value ;
	    	}
	    }
	    //检测是否松开屏幕
		if(ts.type == EV_KEY && ts.code == BTN_TOUCH)
		{
			if(ts.value == 0)
			{
				break;
			}
		}
	}
	printf("x %d  y  %d\n",x,y);
	*tx = x;
	*ty = y;
	return 0;
}
