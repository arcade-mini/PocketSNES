# Define the applications properties here:

TARGET = ./dist/PocketSNES.dge

CHAINPREFIX := /opt/rs97-fixed-toolchain
CROSS_COMPILE := $(CHAINPREFIX)/usr/bin/mipsel-linux-

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

SYSROOT := /opt/rs97-fixed-toolchain/mipsel-buildroot-linux-musl/sysroot/
SDL_CFLAGS := -I/opt/rs97-fixed-toolchain/mipsel-buildroot-linux-musl/sysroot/usr/include/SDL
SDL_LIBS := -lSDL

INCLUDE = -I pocketsnes \
		-I sal/linux/include -I sal/include \
		-I pocketsnes/include \
		-I menu -I pocketsnes/linux -I pocketsnes/snes9x

CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ -DFOREVER_16_BIT  $(SDL_CFLAGS)
# CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ $(SDL_CFLAGS)
CFLAGS += -Ofast -fdata-sections -ffunction-sections -mips32 -march=mips32 -mno-mips16 -mno-abicalls -fno-PIC -fomit-frame-pointer -fno-builtin
CFLAGS += -fno-common -Wno-write-strings -Wno-sign-compare -ftree-vectorize
CFLAGS += -funswitch-loops -fno-strict-aliasing
CFLAGS += -mno-relax-pic-calls -mlong32 -mlocal-sdata -mframe-header-opt -mno-check-zero-division -mfp32 -mgp32 -mno-embedded-data -fno-pic -mno-interlink-compressed -mno-mt -mno-micromips -mno-interlink-mips16
CFLAGS += -DMIPS_XBURST -DFAST_LSB_WORD_ACCESS
#-DNO_ROM_BROWSER
#CFLAGS += -fprofile-generate -fprofile-dir=/mnt/int_sd/profile
CFLAGS += -fprofile-use

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-math-errno -fno-threadsafe-statics

LDFLAGS =  $(SDL_LIBS) -lpthread -lz -lpng -flto -Wl,--as-needed -Wl,--gc-sections -s -no-pie

# Find all source files
SOURCE = pocketsnes/snes9x menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJS    = $(OBJ_CPP) $(OBJ_C)

.PHONY : all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CMD)$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CMD)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CMD)$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	$(CMD)rm -f $(OBJS) $(TARGET)
	$(CMD)rm -rf .opk_data $(TARGET).opk
