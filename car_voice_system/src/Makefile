# =====================================================================
# 车载语音控制系统 —— 开发板端 Makefile
# 交叉编译器：优先 arm-linux-gcc，其次 arm-none-linux-gnueabi-gcc
# 编译产物：car_voice
# =====================================================================

LIBS = $(wildcard ../lib/*)

# 选择交叉编译器
ifeq ($(origin CC),default)
CC = arm-linux-gcc
endif

CPPFLAGS += -I ./inc
LDFLAGS  += -L ../lib

LDFLAGS += -lxml2
LDFLAGS += -ljpeg
LDFLAGS += -lasound
LDFLAGS += -lz
LDFLAGS += -lvnet
LDFLAGS += -lpthread
LDFLAGS += -lm

# 运行时优先查找本地库
LDFLAGS += -Wl,-rpath=. -Wl,-rpath=../lib

TARGET = car_voice
SRCS   = main.c common.c lcd.c ts.c font.c truetype.c ptt.c

$(TARGET): $(SRCS) $(LIBS)
	$(CC) $(SRCS) -o $@ $(CPPFLAGS) $(LDFLAGS) -Wall

clean:
	rm -f $(TARGET)

.PHONY: clean
