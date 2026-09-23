#include <stdio.h>
#include <unistd.h>
#include <signal.h>

// 程序运行标志，用于捕获Ctrl+C退出
volatile int keep_running = 1;

// 信号处理函数：捕获Ctrl+C中断信号
void exit_handler(int sig)
{
    keep_running = 0;
    printf("\n[提示] 收到停止指令，程序即将退出\n");
}

int main(void)
{
    int cnt = 0;

    // 注册Ctrl+C信号响应
    signal(SIGINT, exit_handler);

    // 启动打印
    printf("========================================\n");
    printf("  ARM开发板运行测试程序\n");
    printf("  编译工具: arm-linux-gcc 交叉编译器\n");
    printf("========================================\n");

    // 主循环：每秒打印一次计数
    while (keep_running)
    {
        printf("[运行中] 已运行 %d 秒\n", cnt++);
        sleep(1); // Linux系统延时1秒
    }

    printf("程序结束，总计运行 %d 秒\n", cnt);
    return 0;
}

