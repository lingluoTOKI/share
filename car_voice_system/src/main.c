/* =====================================================================
 * 车载语音控制系统 —— 主程序
 *
 * 硬件平台 : GEC6818 + 800x480 LCD + 板载LED + 触摸屏
 *
 * 屏幕布局:
 *   Y=0   ~ 56   标题栏（渐变 + 左标题 + 右状态）
 *   Y=56  ~ 380  场景 BMP（铺满整屏）
 *   Y=380 ~ 420  对话条
 *   Y=420 ~ 480  底部按钮条
 * ===================================================================== */

#include "car_voice.h"

/* ---------------- 场景图片路径 ---------------- */
#define IMG_HELLO    "/img/1.bmp"
#define IMG_BYE      "/img/10.bmp"
#define IMG_LED_ON   "/img/light.bmp"
#define IMG_LED_OFF  "/img/light_off.bmp"
#define IMG_AC       "/img/ac.bmp"
#define IMG_WIN      "/img/window.bmp"
#define IMG_MAIN     "/img/10.bmp"

#define REC_SEC     3
#define DEFAULT_IP  "192.168.5.4"

/* ---------------- 全局设备状态 ---------------- */
static int g_temp = 24;
static int g_ac_on = 0;
static int g_led_on = 0;
static int g_win_open = 0;

/* 触摸坐标换算 */
static int ts_to_lcd_x(int raw) { return raw * SCREEN_W / TS_RAW_MAX_X; }
static int ts_to_lcd_y(int raw) { return raw * SCREEN_H / TS_RAW_MAX_Y; }


/* ====================================================================
 *                          基础绘制工具
 * ==================================================================== */

void fill_screen_color(int r, int g, int b)
{
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = b;
            lcd[y*SCREEN_W*4 + x*4 + 1] = g;
            lcd[y*SCREEN_W*4 + x*4 + 2] = r;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }
}

/* 把 bitmap 拷贝到 LCD；bitmap 内部布局为 [R][G][B][A]，
 * LCD 帧缓冲布局为 [B][G][R][A]，blit 时交换 B/R 字节。 */
static void blit_bitmap(bitmap *bm, int dst_x, int dst_y)
{
    int x, y;
    for (y = 0; y < (int)bm->height; y++)
        for (x = 0; x < (int)bm->width; x++)
        {
            int px = dst_x + x;
            int py = dst_y + y;
            if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
            {
                /* 跳过完全透明像素 */
                if (bm->map[y*bm->width*4 + x*4 + 3] == 0) continue;
                lcd[py*SCREEN_W*4 + px*4 + 0] = bm->map[y*bm->width*4 + x*4 + 2];
                lcd[py*SCREEN_W*4 + px*4 + 1] = bm->map[y*bm->width*4 + x*4 + 1];
                lcd[py*SCREEN_W*4 + px*4 + 2] = bm->map[y*bm->width*4 + x*4 + 0];
                lcd[py*SCREEN_W*4 + px*4 + 3] = 0;
            }
        }
}

/* 从 LCD 拷贝一块区域到 bitmap（用于“透明”文字层的底图），
 * 保证 fontPrint 抗锯齿时读到正确的背景色 */
static void blit_lcd_to_bitmap(bitmap *bm, int src_x, int src_y)
{
    int x, y;
    for (y = 0; y < (int)bm->height; y++)
        for (x = 0; x < (int)bm->width; x++)
        {
            int px = src_x + x;
            int py = src_y + y;
            if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
            {
                bm->map[y*bm->width*4 + x*4 + 0] = lcd[py*SCREEN_W*4 + px*4 + 2]; /* R */
                bm->map[y*bm->width*4 + x*4 + 1] = lcd[py*SCREEN_W*4 + px*4 + 1]; /* G */
                bm->map[y*bm->width*4 + x*4 + 2] = lcd[py*SCREEN_W*4 + px*4 + 0]; /* B */
                bm->map[y*bm->width*4 + x*4 + 3] = 255;                           /* A */
            }
        }
}

static void draw_hline(int x0, int x1, int y, int r, int g, int b)
{
    int x;
    if (y < 0 || y >= SCREEN_H) return;
    for (x = x0; x <= x1; x++)
        if (x >= 0 && x < SCREEN_W)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = b;
            lcd[y*SCREEN_W*4 + x*4 + 1] = g;
            lcd[y*SCREEN_W*4 + x*4 + 2] = r;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }
}

/* 估算 UTF-8 文字宽度：中文每字 ≈ 字号，ASCII 每字 ≈ 字号/2 */
static int utf8_width(const char *text, int font_size)
{
    int w = 0;
    const unsigned char *p = (const unsigned char *)text;
    while (*p)
    {
        if (*p < 0x80)                { w += font_size / 2; p += 1; }
        else if ((*p & 0xE0) == 0xC0) { w += font_size;     p += 2; }
        else if ((*p & 0xF0) == 0xE0) { w += font_size;     p += 3; }
        else if ((*p & 0xF8) == 0xF0) { w += font_size;     p += 4; }
        else                          { p += 1; }
    }
    return w;
}

/* 居中打印：把 text 在 bm 中水平居中，y 是文字顶部 */
static void font_print_center(font *ft, bitmap *bm, int y,
                              const char *text, color c, int font_size)
{
    int w = utf8_width(text, font_size);
    int x = ((int)bm->width - w) / 2;
    if (x < 0) x = 0;
    fontPrint(ft, bm, x, y, (char *)text, c, bm->width);
}

/* ====================================================================
 *                          对话条（Y=380 ~ 420）
 * ==================================================================== */
void show_dialog(font *ft, char *text)
{
    int x, y;

    bitmap *bm = createBitmap(DIALOG_W, DIALOG_H, 4);
    if (!bm) return;

    /* 整个对话条背景 + 左侧强调条 */
    for (y = 0; y < DIALOG_H; y++) {
        for (x = 0; x < DIALOG_W; x++) {
            if (x < 3) {
                /* 青色强调条：[R=0, G=140, B=180] */
                bm->map[y*DIALOG_W*4 + x*4 + 0] = 0;
                bm->map[y*DIALOG_W*4 + x*4 + 1] = 140;
                bm->map[y*DIALOG_W*4 + x*4 + 2] = 180;
                bm->map[y*DIALOG_W*4 + x*4 + 3] = 255;
            } else {
                bm->map[y*DIALOG_W*4 + x*4 + 0] = 25;
                bm->map[y*DIALOG_W*4 + x*4 + 1] = 25;
                bm->map[y*DIALOG_W*4 + x*4 + 2] = 30;
                bm->map[y*DIALOG_W*4 + x*4 + 3] = 255;
            }
        }
    }

    /* 文字（左对齐） */
    fontSetSize(ft, 22);
    fontPrint(ft, bm, 16, 8, text, getColor(255, 240, 240, 240), DIALOG_W - 20);

    blit_bitmap(bm, DIALOG_X, DIALOG_Y);
    destroyBitmap(bm);
}

/* ====================================================================
 *                          标题栏（顶部 Y=0 ~ 56）
 * ==================================================================== */
void show_status(font *ft)
{
    int x, y;
    char buf[128];

    bitmap *bm = createBitmap(SCREEN_W, TITLE_H, 4);
    if (!bm) return;

    /* 渐变背景直接画进 bitmap */
    for (y = 0; y < TITLE_H; y++)
    {
        int r = 25 - 10 * y / TITLE_H;
        int g = 40 - 15 * y / TITLE_H;
        int b = 70 - 25 * y / TITLE_H;
        for (x = 0; x < SCREEN_W; x++)
        {
            bm->map[y*SCREEN_W*4 + x*4 + 0] = r;
            bm->map[y*SCREEN_W*4 + x*4 + 1] = g;
            bm->map[y*SCREEN_W*4 + x*4 + 2] = b;
            bm->map[y*SCREEN_W*4 + x*4 + 3] = 255;
        }
    }

    /* 左侧标题 */
    fontSetSize(ft, 26);
    fontPrint(ft, bm, 20, 14, "车载语音助手",
              getColor(255, 180, 220, 255), 240);

    /* 右侧状态 */
    snprintf(buf, sizeof(buf), "车灯:%s  空调:%s(%d度)  车窗:%s",
             g_led_on ? "开" : "关", g_ac_on ? "开" : "关", g_temp,
             g_win_open ? "开" : "关");
    fontSetSize(ft, 20);
    int w = utf8_width(buf, 20);
    int sx = SCREEN_W - w - 20;
    if (sx < 0) sx = 0;
    fontPrint(ft, bm, sx, 18, buf, getColor(255, 255, 255, 255), SCREEN_W - sx);

    blit_bitmap(bm, 0, TITLE_Y);
    destroyBitmap(bm);

    draw_hline(0, SCREEN_W - 1, TITLE_Y + TITLE_H - 1, 100, 140, 200);
}

/* ====================================================================
 *                          底部按钮条（Y=420 ~ 480）
 * ==================================================================== */
void draw_talk_button(font *ft)
{
    int x, y;

    for (y = BTN_Y0; y < BTN_Y1; y++)
    {
        int r = 30 - 10 * (y - BTN_Y0) / (BTN_Y1 - BTN_Y0);
        int g = 90 - 30 * (y - BTN_Y0) / (BTN_Y1 - BTN_Y0);
        int b = 160 - 50 * (y - BTN_Y0) / (BTN_Y1 - BTN_Y0);
        for (x = 0; x < SCREEN_W; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = b;
            lcd[y*SCREEN_W*4 + x*4 + 1] = g;
            lcd[y*SCREEN_W*4 + x*4 + 2] = r;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }
    }

    fontSetSize(ft, 26);
    bitmap *bm = createBitmapWithInit(BTN_X1 - BTN_X0, BTN_Y1 - BTN_Y0, 4,
                                      getColor(255, 30, 90, 160));
    font_print_center(ft, bm, 18, "按回车键说话", getColor(255, 255, 255, 255), 26);
    blit_bitmap(bm, BTN_X0, BTN_Y0);
    destroyBitmap(bm);

    draw_hline(0, SCREEN_W - 1, BTN_Y0, 120, 180, 240);
}

/* ====================================================================
 *                          启动按钮（居中）
 * ==================================================================== */
void draw_start_button(font *ft)
{
    int x, y;

    /* 阴影 */
    for (y = START_BTN_Y0 + 3; y < START_BTN_Y1 + 3; y++)
        for (x = START_BTN_X0 + 3; x < START_BTN_X1 + 3; x++)
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H)
            {
                lcd[y*SCREEN_W*4 + x*4 + 0] = 10;
                lcd[y*SCREEN_W*4 + x*4 + 1] = 15;
                lcd[y*SCREEN_W*4 + x*4 + 2] = 25;
                lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
            }

    /* 主体 */
    for (y = START_BTN_Y0; y < START_BTN_Y1; y++)
        for (x = START_BTN_X0; x < START_BTN_X1; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = 0;
            lcd[y*SCREEN_W*4 + x*4 + 1] = 140;
            lcd[y*SCREEN_W*4 + x*4 + 2] = 255;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }

    /* 文字居中 */
    fontSetSize(ft, 36);
    bitmap *bm = createBitmapWithInit(START_BTN_X1 - START_BTN_X0,
                                      START_BTN_Y1 - START_BTN_Y0, 4,
                                      getColor(255, 255, 140, 0));
    font_print_center(ft, bm, 12, "启 动", getColor(255, 255, 255, 255), 36);
    blit_bitmap(bm, START_BTN_X0, START_BTN_Y0);
    destroyBitmap(bm);
}

int wait_start_button(void)
{
    int rx = 0, ry = 0;
    get_xy(&rx, &ry);
    int x = ts_to_lcd_x(rx);
    int y = ts_to_lcd_y(ry);
    if (x >= START_BTN_X0 && x < START_BTN_X1 &&
        y >= START_BTN_Y0 && y < START_BTN_Y1)
        return 1;
    return 0;
}

/* 底部按钮条任意位置点击即返回 1 */
int wait_talk_button(void)
{
    int rx = 0, ry = 0;
    get_xy(&rx, &ry);
    int x = ts_to_lcd_x(rx);
    int y = ts_to_lcd_y(ry);
    if (x >= BTN_X0 && x < BTN_X1 && y >= BTN_Y0 && y < BTN_Y1)
        return 1;
    return 0;
}

/* ====================================================================
 *                          场景显示（图片铺满整屏，UI 叠加）
 * ==================================================================== */
static void show_scene_bmp(font *ft, const char *path)
{
    show_bmp((char *)path, 0, 0);
    show_status(ft);
    draw_talk_button(ft);
}

/* ====================================================================
 *                          启动界面
 * ==================================================================== */
void show_start_screen(font *ft)
{
    int x, y, i, dx, dy;

    /* 1. 渐变背景 */
    for (y = 0; y < SCREEN_H; y++)
    {
        int r = 15 - 10 * y / SCREEN_H;
        int g = 25 - 20 * y / SCREEN_H;
        int b = 45 - 35 * y / SCREEN_H;
        for (x = 0; x < SCREEN_W; x++)
        {
            lcd[y*SCREEN_W*4 + x*4 + 0] = b;
            lcd[y*SCREEN_W*4 + x*4 + 1] = g;
            lcd[y*SCREEN_W*4 + x*4 + 2] = r;
            lcd[y*SCREEN_W*4 + x*4 + 3] = 0;
        }
    }

    /* 2. 顶部三个装饰点 */
    int dot_x[3] = { 370, 400, 430 };
    int dot_y = 60;
    int dot_r = 5;
    for (i = 0; i < 3; i++)
    {
        for (dy = -dot_r; dy <= dot_r; dy++)
            for (dx = -dot_r; dx <= dot_r; dx++)
                if (dx*dx + dy*dy <= dot_r*dot_r)
                {
                    int px = dot_x[i] + dx;
                    int py = dot_y + dy;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H)
                    {
                        lcd[py*SCREEN_W*4 + px*4 + 0] = 200;
                        lcd[py*SCREEN_W*4 + px*4 + 1] = 150;
                        lcd[py*SCREEN_W*4 + px*4 + 2] = 100;
                        lcd[py*SCREEN_W*4 + px*4 + 3] = 0;
                    }
                }
    }

    /* 3. 主标题（居中，用 LCD 已画好的渐变作底） */
    bitmap *bm = createBitmap(SCREEN_W, 80, 4);
    blit_lcd_to_bitmap(bm, 0, 100);
    fontSetSize(ft, 52);
    font_print_center(ft, bm, 20, "车载语音",
                      getColor(255, 255, 200, 100), 52);
    blit_bitmap(bm, 0, 100);
    destroyBitmap(bm);

    /* 4. 装饰线 */
    draw_hline(230, 570, 95, 100, 70, 50);
    draw_hline(230, 570, 185, 100, 70, 50);

    /* 5. 副标题（居中） */
    bitmap *bm2 = createBitmap(SCREEN_W, 40, 4);
    blit_lcd_to_bitmap(bm2, 0, 200);
    fontSetSize(ft, 24);
    font_print_center(ft, bm2, 6, "智能车载语音助手",
                      getColor(255, 140, 180, 255), 24);
    blit_bitmap(bm2, 0, 200);
    destroyBitmap(bm2);

    /* 6. 启动按钮 */
    draw_start_button(ft);

    /* 7. 底部功能提示 */
    bitmap *bm4 = createBitmap(SCREEN_W, 30, 4);
    blit_lcd_to_bitmap(bm4, 0, 430);
    fontSetSize(ft, 20);
    font_print_center(ft, bm4, 4, "车灯 · 空调 · 车窗 · 语音控制",
                      getColor(255, 130, 130, 160), 20);
    blit_bitmap(bm4, 0, 430);
    destroyBitmap(bm4);

    /* 8. 对话条提示 */
    show_dialog(ft, "请点击【启动】按钮，连接语音识别引擎");
}

/* ====================================================================
 *                          各场景封装
 * ==================================================================== */

void show_main_screen(font *ft)  { show_scene_bmp(ft, IMG_MAIN); }
void show_hello_screen(font *ft) { show_scene_bmp(ft, IMG_HELLO); }
void show_bye_screen(font *ft)   { show_scene_bmp(ft, IMG_BYE); }

void show_led_scene(font *ft, int on)
{
    show_scene_bmp(ft, on ? IMG_LED_ON : IMG_LED_OFF);
    if (on) { led_ctl(LED_LIGHT_L, 1); led_ctl(LED_LIGHT_R, 1); }
    else    { led_ctl(LED_LIGHT_L, 0); led_ctl(LED_LIGHT_R, 0); }
}

void show_ac_scene(font *ft, int temp, int on)
{
    (void)temp; (void)on;
    show_scene_bmp(ft, IMG_AC);
}

void show_win_scene(font *ft, int open)
{
    (void)open;
    show_scene_bmp(ft, IMG_WIN);
}

void print_greeting(font *ft)
{
    show_dialog(ft, "您好，我是车载语音助手，请问需要什么帮助？");
}

/* LED 控制：2 字节协议 —— buf[0]=状态, buf[1]=灯号
 * 注意：若驱动协议不同请按实际驱动修改 */
int led_ctl(int n, int sta)
{
    int fd = open("/dev/led_drv", O_RDWR);
    if (fd < 0) { printf("led_drv open failed\n"); return -1; }

    char buf[2];
    buf[0] = sta;
    buf[1] = n;
    int ret = write(fd, buf, 2);
    close(fd);
    return (ret == 2) ? 0 : -1;
}

/* 清空 stdin 行缓冲 */
static void wait_enter(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* ====================================================================
 *                          主程序
 * ==================================================================== */
int main(int argc, char *argv[])
{
    const char *server_ip = (argc >= 2) ? argv[1] : DEFAULT_IP;
    int sockfd;

    if (lcd_init() != 0) { printf("lcd init failed\n"); return -1; }
    if (ts_init() != 0)  { printf("ts init failed\n"); lcd_close(); return -1; }

    font *ft = fontLoad("./simfang.ttf");
    if (ft == NULL)
    {
        printf("font load error\n");
        lcd_close();
        ts_close();
        return -1;
    }

    show_start_screen(ft);
    while (wait_start_button() == 0)
        ;

    print_greeting(ft);
    sleep(2);

    sockfd = init_sock(server_ip);

    show_main_screen(ft);
    show_dialog(ft, "按回车键开始录音，请说话");

    while (1)
    {
        int id_num = 0;
        char rec_cmd[160];

        show_dialog(ft, "按回车键开始录音，请说话");
        wait_enter();

        show_dialog(ft, "正在录音，请说话...");
        snprintf(rec_cmd, sizeof(rec_cmd),
                 "arecord -d %d -t raw -f S16_LE -r 16000 -c 1 ./cmd.pcm",
                 REC_SEC);
        int rc = system(rec_cmd);
        if (rc != 0)
        {
            show_dialog(ft, "录音失败，请检查麦克风或 arecord。");
            sleep(2);
            show_main_screen(ft);
            continue;
        }

        send_pcm(sockfd, "./cmd.pcm");

        xmlChar *id = wait4id(sockfd);
        if (id == NULL)
        {
            show_dialog(ft, "抱歉，没有听清，请再说一遍。");
            sleep(1);
            close(sockfd);
            printf("reconnect to server...\n");
            sockfd = init_sock(server_ip);
            show_main_screen(ft);
            continue;
        }
        id_num = atoi((char *)id);
        xmlFree(id);
        printf("recv cmd id: %d\n", id_num);

        switch (id_num)
        {
        case CMD_HELLO:
            show_hello_screen(ft);
            show_dialog(ft, "您好，我是车载语音助手，很高兴为您服务！");
            break;

        case CMD_BYE:
            show_bye_screen(ft);
            show_dialog(ft, "再见，祝您出行愉快！");
            sleep(2);
            goto exit_loop;

        case CMD_LED_ON:
            g_led_on = 1;
            show_led_scene(ft, g_led_on);
            show_dialog(ft, "好的，已为您打开车载大灯。");
            break;

        case CMD_LED_OFF:
            g_led_on = 0;
            show_led_scene(ft, g_led_on);
            show_dialog(ft, "好的，已为您关闭车载大灯。");
            break;

        case CMD_AC_ON:
            g_ac_on = 1;
            show_ac_scene(ft, g_temp, g_ac_on);
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "好的，空调已开启，当前温度 %d 度。", g_temp);
                show_dialog(ft, buf);
            }
            break;

        case CMD_AC_OFF:
            g_ac_on = 0;
            show_ac_scene(ft, g_temp, g_ac_on);
            show_dialog(ft, "好的，已为您关闭空调。");
            break;

        case CMD_AC_HOT:
            if (g_ac_on && g_temp < 32) g_temp += 1;
            show_ac_scene(ft, g_temp, g_ac_on);
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "好的，已调高温度，当前 %d 度。", g_temp);
                show_dialog(ft, buf);
            }
            break;

        case CMD_AC_COOL:
            if (g_ac_on && g_temp > 16) g_temp -= 1;
            show_ac_scene(ft, g_temp, g_ac_on);
            {
                char buf[64];
                snprintf(buf, sizeof(buf), "好的，已调低温度，当前 %d 度。", g_temp);
                show_dialog(ft, buf);
            }
            break;

        case CMD_WIN_OPEN:
            g_win_open = 1;
            show_win_scene(ft, g_win_open);
            show_dialog(ft, "好的，已为您降下车窗。");
            break;

        case CMD_WIN_CLOSE:
            g_win_open = 0;
            show_win_scene(ft, g_win_open);
            show_dialog(ft, "好的，已为您升起车窗。");
            break;

        case CMD_SCENE:
            if (g_led_on)        show_led_scene(ft, 1);
            else if (g_ac_on)    show_ac_scene(ft, g_temp, 1);
            else if (g_win_open) show_win_scene(ft, 1);
            else                 show_main_screen(ft);
            show_dialog(ft, "当前正在显示车载场景，请查看屏幕。");
            break;

        default:
            show_dialog(ft, "抱歉，暂不支持该指令。");
            break;
        }
    }

exit_loop:
    close(sockfd);
    fontUnload(ft);
    lcd_close();
    ts_close();
    return 0;
}