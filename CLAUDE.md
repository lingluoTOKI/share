# CLAUDE.md

本文件为 Claude Code（claude.ai/code）在本仓库中编写代码时提供指导。

## 项目简介

这是一个嵌入式 Linux 课程项目：**基于 GEC210 开发板（三星 S5PV210 ARM 处理器）的语音控制机器人**。系统拆分为两台通过 TCP 通信的机器：

- **ARM 开发板（GEC210）** —— 录制一段短音频，发送给 x86 机器，并根据识别出的指令 id 执行动作（播放音乐、开灯、显示图片等）。
- **x86 / Ubuntu 机器** —— 作为 TCP 服务器运行语音识别引擎（科大讯飞 MSC 离线语法识别），接收 PCM 音频，返回 XML 识别结果。

各目录是本课程的练习内容，最终汇总为 `robot/` 项目。`1文档/` 存放课程笔记（`.docx`，中文），`1文档/项目要求/项目要求.docx` 是任务书。

## 构建方式

在 x86 主机上使用 GEC210 工具链进行交叉编译。

```sh
# ARM 客户端（robot/gec210 目录） -> 生成 voicectl
make            # 使用 arm-none-linux-gnueabi-gcc
make clean

# x86 语音识别引擎（robot/x86/examples/asr_record_demo 目录）
# -> 在 robot/x86/bin/ 中生成 main（使用本机 gcc，CROSS_COMPILE 为空）
make
```

独立练习目录（`lcd/`、`ts/`、`led/`、`trueType/`）**没有 Makefile** —— 其源文件是用 `arm-none-linux-gnueabi-gcc` 手工编译的，仓库里只保留了已编译好的 ARM 可执行文件。

## 架构

### 双机通信流程（`robot/` 项目）

```
ARM（robot/gec210/voicectl）                   x86（robot/x86/examples/asr_record_demo）
  arecord -d3 -c1 -r16000 -traw -fS16_LE       根据 cmd.bnf 构建语法（科大讯飞 MSC）
      -> cmd.pcm                                监听 TCP 端口 54321
  send_pcm(sockfd, cmd.pcm)  --------------->   接收 PCM，执行离线语音识别
  wait4id(sockfd)           <---------------    返回 XML 结果（如 <cmd id="2">你好</cmd>）
  解析 XML，atoi(id)，根据 id 执行动作
```

运行顺序：先在 x86 上启动识别引擎（`robot/x86/bin/main <gec210-ip>`），再运行 ARM 客户端（`./voicectl <ubuntu-ip>`）。

- **`robot/gec210/voicectl.c`** —— ARM 客户端主循环：录音 → 发送 PCM → 等待 id → 根据 id 分支处理。`if(id_num == N)` 代码块即新增语音指令绑定动作的位置。
- **`robot/gec210/common.c`** —— 共享辅助函数库（直接参与编译，非独立 .so）：socket 封装（`init_sock`、`send_pcm`、`wait4id`）、libxml2 解析（`parse_xml` 读取 `result.xml` 并返回 `<cmd id=...>` 属性，置信度低于 30 时拒绝）、JPEG 解码与"百叶窗"淡入淡出过渡、framebuffer 初始化。
- **`robot/gec210/inc/common.h`** —— 核心头文件：包含所有依赖并声明 socket/XML/触摸/显示的 API。`DEF_PORT` 为 54321，`WIDTH/HEIGHT` = 800×480。

### 硬件 / 驱动接口

| 设备 | 路径 | 使用者 |
|---|---|---|
| LCD 帧缓冲 | `/dev/fb0` | `lcd.c`（`lcd_init` mmap 800×480×4，BGRA） |
| 触摸屏 | `/dev/input/event0` | `ts.c`（`get_xy` 阻塞等待 `BTN_TOUCH` 抬起） |
| LED | `/dev/led_drv` | `led/led_one.c`（写入 2 字节：`{状态, 灯号}`）；`led/led_drv.ko` 为内核模块 |
| 音频 | ALSA（`arecord` / `libasound`） | `voicectl.c` 录制 3 秒单声道 16kHz 16 位 LE |

帧缓冲按 BGRA 顺序直接写入（`lcd[y*800*4 + x*4 + 0..3]` = B,G,R,A）。BMP 显示（`show_bmp`）读取 24 位 BMP 并垂直翻转。

### 字体渲染

`font.c`/`font.h` 封装了内置的 **stb_truetype**（`truetype.c`/`truetype.h`），将 `.ttf` 字形渲染到内存中的 RGBA `bitmap`，再拷贝到帧缓冲。API 流程：`fontLoad` → `fontSetSize` → `createBitmap(WithInit)` → `fontPrint` → 拷贝到 `lcd` → `destroyBitmap`/`fontUnload`。

### 语音识别引擎（x86）

`robot/x86/examples/asr_record_demo/`（`asr_record_demo.c`、`speech_recognizer.c`、`linuxrec.c`）链接科大讯飞的 **`libmsc.so`**（头文件在 `robot/x86/lib/inc/`：`qisr.h`、`msp_cmn.h`、`msp_errors.h`）。离线语法定义在 `robot/x86/bin/cmd.bnf`：

```
你好!id(2) | 再见!id(999) | 显示一张图片!id(3) | 现在几点!id(10) | 播放音乐!id(11) | 开灯!id(12)
```

新增语音指令的方法：在 `cmd.bnf` 的 `<cmd>` 候选项中扩展（重新构建语法），并在 `voicectl.c` 中添加对应的 `if(id_num == N)` 分支。识别出的 id 以 `<cmd id="N">…</cmd>` 形式包含在 XML 中，由 `parse_xml` 读取。

### 预编译库（已入库，无源码）

- `robot/gec210/lib/` —— ARM `.so` 库：`libasound`、`libjpeg`、`libxml2`、`libz`、`libvnet`；对应头文件在 `robot/gec210/inc/{alsa,libxml}`。
- `robot/x86/lib/` —— x86 的 `libasound.so` 与 `libmsc.so`。

## 注意事项

- **源码编码不是 UTF-8。** 许多 `.c` 文件（如 `demo.c`、`voicectl.c`）的中文注释为 GBK/GB2312 编码，在 UTF-8 终端中显示为乱码；部分文件带 UTF-8 BOM（`﻿`）。读取和编辑时不要默认按 UTF-8 处理。
- **字体授权**：`simfang.ttf` 同时存在于 `trueType/` 和 `字魂水云行楷(商用需授权)/`。后者的目录名及其中附带的 `.txt` 说明该字体仅供设计交流试用，**商用需购买授权** —— 不要删除或忽略该提示。
- `common.h` 声明了许多函数（通过 `/dev/ttySAC2` 的串口、向 ESP8266 发送 UDP、`send_data_to_8266`）但并非全部已实现 —— 把它当作接口的超集，而非已实际接线的定义。
