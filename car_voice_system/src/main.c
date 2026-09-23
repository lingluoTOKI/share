/* =====================================================================
 * 车载语音控制系统  ——  主程序
 *
 * 硬件平台 : GEC6818 开发板 + 800x480 LCD + 板载LED(7/8/9/10) + 触摸屏
 * 架构     : 开发板作为客户端，通过 socket 连接 Ubuntu 上的离线语音识别引擎
 *            (科大讯飞 SDK)。识别结果以 XML 中的 cmd id 回传，本程序据此
 *            切换场景、控制 LED、显示对话。
 *
 * 场景     : ① 启动界面  ② 语音控制主界面  ③ 车灯场景(LED)
 *            ④ 空调控制场景  ⑤ 车窗控制场景
 * 对话     : 不少于 5 段人机交互对话
 * ===================================================================== */

#include "car_voice.h"
#include "ptt.h"

/* ---------------- 场景图片路径 ----------------
 * 部署时把 res/img 下的 4 张 bmp 放到开发板可执行文件同级的 img/ 目录
 */
#define IMG_LED_ON   "./img/light.bmp"      /* 车灯开启 */
#define IMG_LED_OFF  "./img/light_off.bmp"  /* 车灯关闭 */
#define IMG_AC       "./img/ac.bmp"         /* 空调面板 */
#define IMG_WIN      "./img/window.bmp"     /* 车窗场景 */

/* ---------------- LCD 显示基础封装 ---------------- */

/* 整屏填充一种颜色：r,g,b 取 0~255 */
void fill_screen_color(int r, int g, int b)
{
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
    {
        for (x = 0; x < SCREEN_W; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = b;   /* B */
            lcd[y*SCREEN_W*4 + x*4 + 1] = g;   /* G */
            lcd[y*SCREEN_W*4 + x*4 + 2] = r;   /* R */
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;   /* A */
        }
    }
}

/* 对话展示区：屏幕底部绘制半透明深色条，再写入白色文字 */
void show_dialog(font *ft, char *text)
{
    int x, y;
    /* 底部信息条背景（深色） */
    for (y = DIALOG_Y; y < DIALOG_Y + DIALOG_H; y++)
    {
        for (x = DIALOG_X; x < DIALOG_X + DIALOG_W; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = 30;
            lcd[y*SCREEN_W*4 + x*4 + 1] = 30;
            lcd[y*SCREEN_W*4 + x*4 + 2] = 30;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }
    }

    /* 在信息条上叠加白色文字 */
    fontSetSize(ft, 26);
    bitmap *bm = createBitmapWithInit(DIALOG_W, DIALOG_H, 4, getColor(255, 30, 30, 30));
    fontPrint(ft, bm, 10, 8, text, getColor(255, 255, 255, 255), DIALOG_W);
    for (y = 0; y < bm->height; y++)
    {
        for (x = 0; x < bm->width; x++)
        {
            if (x + DIALOG_X < SCREEN_W && y + DIALOG_Y < SCREEN_H)
            {
                /* 位图 RGBA -> 帧缓冲 BGRA，交换 R/B 通道 */
                lcd[(DIALOG_Y+y)*SCREEN_W*4 + (DIALOG_X+x)*4 + 0] = bm->map[y*bm->width*4 + x*4 + 2]; /* B */
                lcd[(DIALOG_Y+y)*SCREEN_W*4 + (DIALOG_X+x)*4 + 1] = bm->map[y*bm->width*4 + x*4 + 1]; /* G */
                lcd[(DIALOG_Y+y)*SCREEN_W*4 + (DIALOG_X+x)*4 + 2] = bm->map[y*bm->width*4 + x*4 + 0]; /* R */
                lcd[(DIALOG_Y+y)*SCREEN_W*4 + (DIALOG_X+x)*4 + 3] = bm->map[y*bm->width*4 + x*4 + 3]; /* A */
            }
        }
    }
    destroyBitmap(bm);
}

/* ---------------- LED 控制 ---------------- */
int led_ctl(int n, int sta)
{
    int fd = open("/dev/led_drv", O_RDWR);
    if (fd < 0)
    {
        printf("led_drv open failed, please insmod led_drv.ko first\n");
        return -1;
    }
    char buf[2];
    buf[0] = sta;   /* 状态在前 */
    buf[1] = n;     /* 灯号在后 */
    int ret = write(fd, buf, 2);
    close(fd);
    return (ret == 2) ? 0 : -1;
}

/* ---------------- 各场景显示 ---------------- */

/* 启动界面：深色底 + 项目标题 + 操作提示 */
void show_start_screen(font *ft)
{
    fill_screen_color(20, 20, 30);
    fontSetSize(ft, 44);
    bitmap *bm = createBitmapWithInit(SCREEN_W, 100, 4, getColor(0, 0, 0, 0));
    fontPrint(ft, bm, 120, 30, "车载语音控制系统", getColor(255, 255, 120, 0), 600);
    int x, y;
    for (y = 0; y < bm->height; y++)
        for (x = 0; x < bm->width; x++)
            if (bm->map[y*bm->width*4 + x*4 + 0]
                || bm->map[y*bm->width*4 + x*4 + 1]
                || bm->map[y*bm->width*4 + x*4 + 2])
            {
                /* 位图 RGBA -> 帧缓冲 BGRA，交换 R/B 通道 */
                lcd[(150+y)*SCREEN_W*4 + x*4 + 0] = bm->map[y*bm->width*4 + x*4 + 2]; /* B */
                lcd[(150+y)*SCREEN_W*4 + x*4 + 1] = bm->map[y*bm->width*4 + x*4 + 1]; /* G */
                lcd[(150+y)*SCREEN_W*4 + x*4 + 2] = bm->map[y*bm->width*4 + x*4 + 0]; /* R */
            }
    destroyBitmap(bm);
    show_dialog(ft, "正在启动语音控制系统，请稍候...");
}

/* 语音控制主界面：列出可用的语音指令 + "启动语音控制"按钮 */
void show_main_screen(font *ft)
{
    fill_screen_color(35, 35, 50);
    fontSetSize(ft, 28);
    bitmap *bm = createBitmapWithInit(SCREEN_W, 320, 4, getColor(0, 0, 0, 0));
    fontPrint(ft, bm, 60, 20,  "语音指令列表", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 60,  "1. 你好           2. 再见", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 100, "3. 打开车灯       4. 关闭车灯", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 140, "5. 打开空调       6. 关闭空调", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 180, "7. 空调升温       8. 空调降温", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 220, "9. 打开车窗       10. 关闭车窗", getColor(255, 255, 255, 255), 700);
    fontPrint(ft, bm, 60, 260, "11. 显示当前场景", getColor(255, 255, 255, 255), 700);
    int x, y;
    for (y = 0; y < bm->height; y++)
        for (x = 0; x < bm->width; x++)
            if (bm->map[y*bm->width*4 + x*4 + 0]
                || bm->map[y*bm->width*4 + x*4 + 1]
                || bm->map[y*bm->width*4 + x*4 + 2])
            {
                /* 位图 RGBA -> 帧缓冲 BGRA，交换 R/B 通道 */
                lcd[(60+y)*SCREEN_W*4 + x*4 + 0] = bm->map[y*bm->width*4 + x*4 + 2]; /* B */
                lcd[(60+y)*SCREEN_W*4 + x*4 + 1] = bm->map[y*bm->width*4 + x*4 + 1]; /* G */
                lcd[(60+y)*SCREEN_W*4 + x*4 + 2] = bm->map[y*bm->width*4 + x*4 + 0]; /* R */
            }
    destroyBitmap(bm);

    /* 启动语音控制按钮（绿色矩形） */
    for (y = BTN_Y0; y < BTN_Y1; y++)
        for (x = BTN_X0; x < BTN_X1; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = 40;   /* B */
            lcd[y*SCREEN_W*4 + x*4 + 1] = 120;  /* G */
            lcd[y*SCREEN_W*4 + x*4 + 2] = 30;   /* R */
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;    /* A */
        }

    /* 按钮文字标签 */
    bitmap *lb = createBitmapWithInit(BTN_X1 - BTN_X0, BTN_Y1 - BTN_Y0, 4, getColor(0, 0, 0, 0));
    fontPrint(ft, lb, 36, 14, "启动语音控制", getColor(255, 255, 255, 255), BTN_X1 - BTN_X0);
    for (y = 0; y < lb->height; y++)
        for (x = 0; x < lb->width; x++)
            if (lb->map[y*lb->width*4 + x*4 + 0]
                || lb->map[y*lb->width*4 + x*4 + 1]
                || lb->map[y*lb->width*4 + x*4 + 2])
            {
                lcd[(BTN_Y0+y)*SCREEN_W*4 + (BTN_X0+x)*4 + 0] = lb->map[y*lb->width*4 + x*4 + 2]; /* B */
                lcd[(BTN_Y0+y)*SCREEN_W*4 + (BTN_X0+x)*4 + 1] = lb->map[y*lb->width*4 + x*4 + 1]; /* G */
                lcd[(BTN_Y0+y)*SCREEN_W*4 + (BTN_X0+x)*4 + 2] = lb->map[y*lb->width*4 + x*4 + 0]; /* R */
            }
    destroyBitmap(lb);

    show_dialog(ft, "请触摸下方按钮开始语音控制");
}

/* 车灯场景：on=1 显示亮灯图并点亮LED，on=0 显示熄灯图并熄灭LED */
void show_led_scene(font *ft, int on)
{
    if (on)
    {
        show_bmp(IMG_LED_ON, 0, 0);
        led_ctl(LED_LIGHT_L, 1);
        led_ctl(LED_LIGHT_R, 1);
        show_dialog(ft, "好的，已为您打开车载大灯。");
    }
    else
    {
        show_bmp(IMG_LED_OFF, 0, 0);
        led_ctl(LED_LIGHT_L, 0);
        led_ctl(LED_LIGHT_R, 0);
        show_dialog(ft, "好的，已为您关闭车载大灯。");
    }
}

/* 空调场景：显示空调面板图，对话条提示温度与开关状态 */
void show_ac_scene(font *ft, int temp, int on)
{
    show_bmp(IMG_AC, 0, 0);
    if (on)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "空调已开启，当前温度 %d 度。", temp);
        show_dialog(ft, buf);
    }
    else
    {
        show_dialog(ft, "好的，已为您关闭空调。");
    }
}

/* 车窗场景：显示车窗图，open=1 降窗，open=0 升窗 */
void show_win_scene(font *ft, int open)
{
    show_bmp(IMG_WIN, 0, 0);
    show_dialog(ft, open ? "好的，已为您降下车窗。" : "好的，已为您升起车窗。");
}

/* 开机问候语（满足 5 段对话之一） */
void print_greeting(font *ft)
{
    show_dialog(ft, "您好，我是车载语音助手，请问需要什么帮助？");
}

/* 等待触摸按下启动按钮区域 */
int wait_touch_button(void)
{
    int tx, ty;
    while (1)
    {
        if (get_xy(&tx, &ty) == 0)
        {
            if (tx >= BTN_X0 && tx <= BTN_X1 && ty >= BTN_Y0 && ty <= BTN_Y1)
                return 0;
        }
    }
}

/* ---------------- 主程序 ---------------- */
int main(int argc, char *argv[])
{
    int temp = 24;          /* 空调默认温度 */
    int ac_on = 0;          /* 空调开关状态 */
    int led_on = 0;         /* 车灯开关状态 */
    int win_open = 0;       /* 车窗状态 */

    /* 1. 初始化 LCD */
    lcd_init();

    /* 2. 初始化触摸屏 */
    ts_init();

    /* 3. 加载中文字库 */
    font *ft = fontLoad("./simfang.ttf");
    if (ft == NULL)
    {
        printf("font load error, need ./simfang.ttf\n");
        lcd_close();
        return -1;
    }

    /* 4. 显示启动界面 */
    show_start_screen(ft);
    sleep(2);
    print_greeting(ft);      /* 对话①：开机问候 */
    sleep(2);

    /* 5. 进入主界面，等待触摸启动语音 */
    show_main_screen(ft);
    wait_touch_button();

    /* 6. 建立与 Ubuntu 语音识别引擎的连接
     *    argc>1 时用传入的服务器 IP，否则默认 192.168.5.4 */
    int sockfd;
    if (argc >= 2)
        sockfd = init_sock(argv[1]);
    else
        sockfd = init_sock("192.168.5.4");

    show_dialog(ft, "语音控制已启动，请说出指令...");

    /* 7. 语音识别主循环（按住"按住说话"按钮控制录音时长） */
    while (1)
    {
        int id_num = 0;

        /* 每轮重绘 PTT 按钮（场景图会覆盖它），并提示 */
        draw_ptt_button(ft, "按住说话");
        show_dialog(ft, "请按住下方按钮说话，松开即识别。");

        /* 等待在 PTT 区域按下 */
        touch_wait_press(PTT_X0, PTT_Y0, PTT_X1, PTT_Y1);
        show_dialog(ft, "正在聆听，松开结束...");

        /* 开始录音；等待松开，最长 PTT_MAX_SEC 秒 */
        recorder_start("./cmd.pcm");
        int released = touch_wait_release(PTT_MAX_SEC);
        recorder_stop();

        if (!released)
            show_dialog(ft, "已达到最长录音时间，正在识别...");

        /* 录音太短（不足约0.5秒）则重试 */
        if (recorder_frames() < PTT_MIN_FRAMES)
        {
            show_dialog(ft, "说话时间太短，请重试。");
            sleep(1);
            continue;
        }

        /* 发送 PCM 给语音识别引擎 */
        send_pcm(sockfd, "./cmd.pcm");

        /* 接收识别结果 XML 中的 cmd id */
        xmlChar *id = wait4id(sockfd);
        if (id == NULL)
        {
            show_dialog(ft, "没有听清，请再说一遍。");
            sleep(1);
            continue;
        }
        id_num = atoi((char *)id);

        printf("recv cmd id: %d\n", id_num);

        switch (id_num)
        {
        case CMD_HELLO:                       /* 对话②：问候 */
            show_main_screen(ft);
            show_dialog(ft, "您好，我在呢！可以控制车灯、空调、车窗。");
            break;

        case CMD_BYE:                         /* 对话③：再见退出 */
            show_dialog(ft, "再见，祝您出行愉快！");
            sleep(2);
            goto exit_loop;

        case CMD_LED_ON:                      /* 打开车灯 */
            led_on = 1;
            show_led_scene(ft, led_on);
            break;

        case CMD_LED_OFF:                     /* 关闭车灯 */
            led_on = 0;
            show_led_scene(ft, led_on);
            break;

        case CMD_AC_ON:                       /* 打开空调 */
            ac_on = 1;
            show_ac_scene(ft, temp, ac_on);
            break;

        case CMD_AC_OFF:                      /* 关闭空调 */
            ac_on = 0;
            show_ac_scene(ft, temp, ac_on);
            break;

        case CMD_AC_HOT:                      /* 空调升温 */
            if (ac_on && temp < 32) temp += 1;
            show_ac_scene(ft, temp, ac_on);
            break;

        case CMD_AC_COOL:                     /* 空调降温 */
            if (ac_on && temp > 16) temp -= 1;
            show_ac_scene(ft, temp, ac_on);
            break;

        case CMD_WIN_OPEN:                    /* 打开车窗 */
            win_open = 1;
            show_win_scene(ft, win_open);
            break;

        case CMD_WIN_CLOSE:                   /* 关闭车窗 */
            win_open = 0;
            show_win_scene(ft, win_open);
            break;

        case CMD_SCENE:                       /* 显示当前场景 */
            if (led_on)      show_led_scene(ft, 1);
            else if (ac_on)  show_ac_scene(ft, temp, 1);
            else if (win_open) show_win_scene(ft, 1);
            else             show_main_screen(ft);
            break;

        default:
            show_dialog(ft, "抱歉，暂不支持该指令。");
            break;
        }
    }

exit_loop:
    /* 关闭资源 */
    close(sockfd);
    fontUnload(ft);
    lcd_close();
    ts_close();
    return 0;
}
