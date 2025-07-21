# Flexible Makefile for x64 and arm (ARCH variable)

ARCH ?= x64
GCC_ARM_ROOT = /opt/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu
SYSROOT=/home/chenhequn/rk3588-sysroot

ifeq ($(ARCH),arm)
CC = $(GCC_ARM_ROOT)/bin/aarch64-none-linux-gnu-gcc
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
	-march=armv8-a \
	--sysroot=$(SYSROOT)
LDFLAGS = -Lthirdpart/lib/aarch64 -L$(SYSROOT)/usr/lib/aarch64-linux-gnu -ltensorflow-lite -lm -lasound -lpthread
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

all: lib dtln capture_test
lib:
	rm -rf $(LIB_PATH)
	mkdir -p $(LIB_PATH)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(LIB_PATH)/$(LIBNAME)

clean:
	rm -rf $(LIB_PATH)

.PHONY: all lib clean 

dtln: lib
	rm -rf bin
	mkdir -p bin
ifeq ($(ARCH),arm)
	$(GCC_ARM_ROOT)/bin/aarch64-none-linux-gnu-g++ \
		examples/ns.c \
		--sysroot=$(SYSROOT) \
		-Iinc \
		-Ithirdpart/include \
		-Llib/aarch64 \
		-Lthirdpart/lib/aarch64 \
		-L$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-lathena \
		-lsndfile \
		-lpthread \
		-ltensorflow-lite \
		-ldl \
		-lm \
		-Wl,-rpath,./lib/aarch64 \
		-Wl,-rpath,./thirdpart/lib/aarch64 \
		-Wl,-rpath,$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-no-pie \
		-Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-Wl,-rpath-link,./thirdpart/lib/aarch64 \
		-o bin/dtln
else
	g++ \
		examples/ns.c \
		-Iinc \
		-Ithirdpart/include \
		-Llib/x86_64 \
		-Lthirdpart/lib/x86_64 \
		-lathena \
		-lsndfile \
		-lpthread \
		-ldl \
		-lm \
		-Wl,-rpath,./lib/x86_64 \
		-no-pie \
		-o bin/dtln
endif

capture_test: lib
	rm -rf bin
	mkdir -p bin
ifeq ($(ARCH),arm)
	$(GCC_ARM_ROOT)/bin/aarch64-none-linux-gnu-g++ \
		examples/capture_test.c \
		examples/resample.c \
		--sysroot=$(SYSROOT) \
		-Iinc \
		-Ithirdpart/include \
		-Llib/aarch64 \
		-Lthirdpart/lib/aarch64 \
		-L$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-lathena \
		-lasound \
		-lpthread \
		-ltensorflow-lite \
		-ldl \
		-lm \
		-Wl,-rpath,./lib/aarch64 \
		-Wl,-rpath,./thirdpart/lib/aarch64 \
		-Wl,-rpath,$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-no-pie \
		-Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu \
		-Wl,-rpath-link,./thirdpart/lib/aarch64 \
		-o bin/capture_test
else
	g++ \
		examples/capture_test.c \
		examples/resample.c \
		-Iinc \
		-Ithirdpart/include \
		-Llib/x86_64 \
		-Lthirdpart/lib/x86_64 \
		-lathena \
		-lasound \
		-lpthread \
		-ldl \
		-lm \
		-Wl,-rpath,./lib/x86_64 \
		-no-pie \
		-o bin/capture_test
endif

.PHONY: dtln