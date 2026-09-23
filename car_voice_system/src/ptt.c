/* =====================================================================
 * "按住说话"模块（Push-To-Talk）
 *
 * - 手指按下 PTT 按钮：录音线程启动，持续采集 16kHz/16bit/单声道 裸PCM
 * - 手指松开（或达到最长10秒）：停止录音，文件就绪后发送识别
 *
 * 录音在独立线程中进行，主线程同时检测触摸按下/松开。
 * ===================================================================== */

#include "ptt.h"
#include "alsa/asoundlib.h"

/* ---------------- 录音线程相关 ---------------- */
static snd_pcm_t      *cap = NULL;
static pthread_t       rec_tid;
static volatile int    rec_running = 0;
static FILE           *pcm_fp = NULL;
static volatile long   rec_frames = 0;

#define CHUNK_FRAMES 1600   /* 每次读取 0.1 秒（16000*0.1） */

static void *record_routine(void *arg)
{
    short buf[CHUNK_FRAMES];

    while (rec_running)
    {
        int n = snd_pcm_readi(cap, buf, CHUNK_FRAMES);
        if (n > 0)
        {
            if (pcm_fp)
                fwrite(buf, sizeof(short), n, pcm_fp);
            rec_frames += n;
        }
        else if (n == -EPIPE)   /* 缓冲区溢出，恢复即可 */
        {
            snd_pcm_prepare(cap);
        }
    }
    return NULL;
}

int recorder_start(char *pcmfile)
{
    rec_frames = 0;
    pcm_fp = fopen(pcmfile, "wb");
    if (pcm_fp == NULL)
    {
        perror("open pcm file failed");
        return -1;
    }

    /* 打开默认录音设备 */
    if (snd_pcm_open(&cap, "default", SND_PCM_STREAM_CAPTURE, 0) < 0)
    {
        printf("snd_pcm_open failed\n");
        fclose(pcm_fp);
        pcm_fp = NULL;
        return -1;
    }

    /* 配置硬件参数 */
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(cap, params);
    snd_pcm_hw_params_set_access(cap, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(cap, params, SND_PCM_FORMAT_S16_LE);

    unsigned int rate = 16000;
    int dir = 0;
    snd_pcm_hw_params_set_rate_near(cap, params, &rate, &dir);

    unsigned int ch = 1;
    snd_pcm_hw_params_set_channels_near(cap, params, &ch);

    if (snd_pcm_hw_params(cap, params) < 0)
    {
        printf("snd_pcm_hw_params failed\n");
        snd_pcm_close(cap);
        fclose(pcm_fp);
        pcm_fp = NULL;
        return -1;
    }
    snd_pcm_prepare(cap);

    /* 启动录音线程 */
    rec_running = 1;
    pthread_create(&rec_tid, NULL, record_routine, NULL);
    return 0;
}

void recorder_stop(void)
{
    rec_running = 0;
    pthread_join(rec_tid, NULL);

    if (cap)
    {
        snd_pcm_drain(cap);
        snd_pcm_close(cap);
        cap = NULL;
    }
    if (pcm_fp)
    {
        fflush(pcm_fp);
        fclose(pcm_fp);
        pcm_fp = NULL;
    }
}

long recorder_frames(void)
{
    return rec_frames;
}

/* ---------------- 触摸按下/松开检测 ---------------- */
static int ev_fd = -1;

static int ev_init(void)
{
    if (ev_fd < 0)
        ev_fd = open("/dev/input/event0", O_RDONLY);
    return ev_fd;
}

/* 等待在指定矩形区域内检测到按下 */
int touch_wait_press(int x0, int y0, int x1, int y1)
{
    ev_init();
    struct input_event e;
    int x = -1, y = -1;

    while (1)
    {
        bzero(&e, sizeof(e));
        read(ev_fd, &e, sizeof(e));

        if (e.type == EV_ABS && e.code == ABS_X)
            x = e.value;
        if (e.type == EV_ABS && e.code == ABS_Y)
            y = e.value;

        int pressed = 0;
        if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 1)
            pressed = 1;
        if (e.type == EV_ABS && e.code == ABS_PRESSURE && e.value > 0)
            pressed = 1;

        if (pressed && x >= x0 && x <= x1 && y >= y0 && y <= y1)
            return 0;
    }
}

/* 等待松开，带超时；返回1=已松开，0=超时 */
int touch_wait_release(int timeout_sec)
{
    struct input_event e;
    time_t t0 = time(NULL);

    while (time(NULL) - t0 < timeout_sec)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(ev_fd, &rfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;   /* 200ms 轮询，保证超时判断 */

        int r = select(ev_fd + 1, &rfds, NULL, NULL, &tv);
        if (r > 0)
        {
            bzero(&e, sizeof(e));
            read(ev_fd, &e, sizeof(e));

            if (e.type == EV_KEY && e.code == BTN_TOUCH && e.value == 0)
                return 1;
            if (e.type == EV_ABS && e.code == ABS_PRESSURE && e.value == 0)
                return 1;
        }
    }
    return 0;
}

/* ---------------- 绘制"按住说话"按钮 ---------------- */
void draw_ptt_button(font *ft, char *label)
{
    int x, y;

    /* 按钮底色：醒目的橙红色矩形 */
    for (y = PTT_Y0; y < PTT_Y1; y++)
    {
        for (x = PTT_X0; x < PTT_X1; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = 0;    /* B */
            lcd[y*SCREEN_W*4 + x*4 + 1] = 120;  /* G */
            lcd[y*SCREEN_W*4 + x*4 + 2] = 230;  /* R */
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;    /* A */
        }
    }

    /* 按钮文字标签（白色居中），位图RGBA->帧缓冲BGRA交换R/B */
    int bw = PTT_X1 - PTT_X0;
    int bh = PTT_Y1 - PTT_Y0;
    bitmap *lb = createBitmapWithInit(bw, bh, 4, getColor(0, 0, 0, 0));
    fontSetSize(ft, 32);
    /* 4个汉字约 4*32=128 宽，居中放置 */
    fontPrint(ft, lb, (bw - 128)/2, (bh - 40)/2, label,
              getColor(255, 255, 255, 255), bw);

    for (y = 0; y < lb->height; y++)
    {
        for (x = 0; x < lb->width; x++)
        {
            if (lb->map[y*lb->width*4 + x*4 + 0]
                || lb->map[y*lb->width*4 + x*4 + 1]
                || lb->map[y*lb->width*4 + x*4 + 2])
            {
                lcd[(PTT_Y0+y)*SCREEN_W*4 + (PTT_X0+x)*4 + 0] = lb->map[y*lb->width*4 + x*4 + 2];
                lcd[(PTT_Y0+y)*SCREEN_W*4 + (PTT_X0+x)*4 + 1] = lb->map[y*lb->width*4 + x*4 + 1];
                lcd[(PTT_Y0+y)*SCREEN_W*4 + (PTT_X0+x)*4 + 2] = lb->map[y*lb->width*4 + x*4 + 0];
            }
        }
    }
    destroyBitmap(lb);
}
