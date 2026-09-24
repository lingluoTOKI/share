#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>

int tx, ty;
int ts_fd;

/* 触摸屏初始化 */
int ts_init(void)
{
	ts_fd = open("/dev/input/event0", O_RDWR);
	if(ts_fd < 0)
	{
		perror("open ts failed");
		return -1;
	}
	return 0;
}

/* 关闭触摸屏 */
int ts_close(void)
{
	close(ts_fd);
	return 0;
}

/* 获取一次触摸坐标（松手时返回，返回 0 表示成功）
 * 兼容两种“松手”事件：
 *   1) EV_KEY / BTN_TOUCH / value == 0
 *   2) EV_ABS / ABS_PRESSURE / value == 0
 */
int get_xy(int *tx, int *ty)
{
	int x = -1, y = -1;
	struct input_event ts;

	/* 第一步：等待“按下”事件，并拿到坐标 */
	while (1)
	{
		if (read(ts_fd, &ts, sizeof(ts)) != sizeof(ts))
			continue;

		if (ts.type == EV_ABS)
		{
			if (ts.code == ABS_X) x = ts.value;
			else if (ts.code == ABS_Y) y = ts.value;
			else if (ts.code == ABS_PRESSURE && ts.value > 0) break;
		}
		if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 1)
			break;
	}

	/* 第二步：继续读取直到“松手” */
	while (1)
	{
		if (read(ts_fd, &ts, sizeof(ts)) != sizeof(ts))
			continue;

		if (ts.type == EV_ABS)
		{
			if (ts.code == ABS_X) x = ts.value;
			else if (ts.code == ABS_Y) y = ts.value;
			else if (ts.code == ABS_PRESSURE && ts.value == 0) break;
		}
		if (ts.type == EV_KEY && ts.code == BTN_TOUCH && ts.value == 0)
			break;
	}

	if (x < 0) x = 0;
	if (y < 0) y = 0;

	printf("x %d  y  %d\n", x, y);
	*tx = x;
	*ty = y;
	return 0;
}