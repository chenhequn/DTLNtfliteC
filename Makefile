# Flexible Makefile for x64 and arm (ARCH variable)

ARCH ?= x64

ifeq ($(ARCH),arm)
CC = ~/01_project/rk3588_linux_tve1206r/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
CFLAGS = -fPIC -shared -O3 \
	-Iinc \
	-Iinc/dios_ssp_aec/dios_ssp_aec_tde \
	-Iinc/dios_ssp_aec \
	-Iinc/dios_ssp_agc \
	-Iinc/dios_ssp_doa \
	-Iinc/dios_ssp_gsc \
	-Iinc/dios_ssp_hpf \
	-Iinc/dios_ssp_mvdr \
	-Iinc/dios_ssp_ns \
	-Iinc/dios_ssp_dtln \
	-Iinc/dios_ssp_share \
	-Iinc/dios_ssp_vad \
	-Isrc \
	-Ithirdpart/include \
	-march=armv8-a
LDFLAGS = -Lthirdpart/lib -Lthirdpart/lib/aarch64 -ltensorflow-lite -lm -lasound -lpthread
LIB_PATH = lib/aarch64
LIBNAME = libathena.so

else
CC = gcc
CFLAGS = -fPIC -shared -O3 \
	-Iinc \
	-Iinc/dios_ssp_aec/dios_ssp_aec_tde \
	-Iinc/dios_ssp_aec \
	-Iinc/dios_ssp_agc \
	-Iinc/dios_ssp_doa \
	-Iinc/dios_ssp_gsc \
	-Iinc/dios_ssp_hpf \
	-Iinc/dios_ssp_mvdr \
	-Iinc/dios_ssp_ns \
	-Iinc/dios_ssp_dtln \
	-Iinc/dios_ssp_share \
	-Iinc/dios_ssp_vad \
	-Isrc \
	-Ithirdpart/include
LDFLAGS = -Lthirdpart/lib/x86_64 -ltensorflow-lite -lm -lasound -lpthread
LIB_PATH = lib/x86_64
LIBNAME = libathena.so
endif

SRC = \
	src/dios_ssp_aec/dios_ssp_aec_tde/*.c \
	src/dios_ssp_aec/*.c \
	src/dios_ssp_agc/dios_ssp_agc_api.c \
	src/dios_ssp_doa/dios_ssp_doa_api.c \
	src/dios_ssp_doa/dios_ssp_doa_win.c \
	src/dios_ssp_gsc/*.c \
	src/dios_ssp_hpf/dios_ssp_hpf_api.c \
	src/dios_ssp_mvdr/*.c \
	src/dios_ssp_ns/*.c \
	src/dios_ssp_dtln/*.c \
	src/dios_ssp_share/*.c \
	src/dios_ssp_vad/*.c \
	src/dios_ssp_api.c

all: lib
lib:
	rm -rf $(LIB_PATH)
	mkdir -p $(LIB_PATH)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(LIB_PATH)/$(LIBNAME)

clean:
	rm -rf $(LIB_PATH)

.PHONY: all lib clean 