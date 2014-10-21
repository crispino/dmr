CROSS_PREFIX=mips-linux-
CC=$(CROSS_PREFIX)gcc
LD=$(CROSS_PREFIX)ld
AR=$(CROSS_PREFIX)ar
STRIP=$(CROSS_PREFIX)strip

DMPLAY_PATH := $(shell pwd)/..

include $(DMPLAY_PATH)/config.mak

DEBUG	= -O2
IFLAGS 	+= -I. -I$(DMPLAY_PATH)/include
LDFLAGS	+= 

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

ifeq ("$(CONFIG_FFMPEG_AAC_DECODER_FIXED)", "yes")
CONFIG_FAAD2=yes
endif

ifeq ("$(CONFIG_FFMPEG_OGG_DECODER_FIXED)", "yes")
CONFIG_LIBOGG=yes
CONFIG_TREMOR=yes
endif

ifeq ("$(CONFIG_I2S)", "yes")
CFLAGS	+= -DCONFIG_I2S
OBJS    += i2s.o
endif

ifeq ("$(CONFIG_RLOG)", "yes")
CFLAGS	+= -DCONFIG_RLOG
IFLAGS 	+= -I$(DMPLAY_PATH)/src/rlog
OBJS    += rlog.o
endif

ifeq ("$(CONFIG_FFMPEG)", "yes")
CFLAGS	+= -DCONFIG_FFMPEG
IFLAGS 	+= -I$(DMPLAY_PATH)/src/codec/ffmpeg/include
LDFLAGS	+= -L$(DMPLAY_PATH)/src/codec/ffmpeg/lib -lavutil -lavcodec -lavformat
OBJS    += ffmpeg.o
endif

ifeq ("$(CONFIG_FAAD2)", "yes")
CFLAGS	+= -DCONFIG_FAAD2
IFLAGS 	+= -I$(DMPLAY_PATH)/src/codec/libfaad2/include
LDFLAGS	+= -L$(DMPLAY_PATH)/src/codec/libfaad2/lib -lfaad
endif

ifeq ("$(CONFIG_LIBOGG)", "yes")
CFLAGS	+= -DCONFIG_LIBOGG
IFLAGS 	+= -I$(DMPLAY_PATH)/src/codec/libogg/include
LDFLAGS	+= -L$(DMPLAY_PATH)/src/codec/libogg/lib -logg
endif

ifeq ("$(CONFIG_TREMOR)", "yes")
CFLAGS	+= -DCONFIG_TREMOR
IFLAGS 	+= -I$(DMPLAY_PATH)/src/codec/tremor/include
LDFLAGS	+= -L$(DMPLAY_PATH)/src/codec/tremor/lib -lvorbisidec
endif

ifeq ("$(CONFIG_SOFTWARE_VOLUME)", "yes")
CFLAGS	+= -DCONFIG_SOFTWARE_VOLUME
endif

NAME=dmplay

all:$(NAME)

$(NAME): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(IFLAGS) $(LDFLAGS) $(OBJS)
	$(STRIP) $@

clean:
	rm -f $(NAME) $(OBJS) $(OUTPOUT_OBJS)

distclean: clean
	-rm -fr codec/ffmpeg codec/libogg codec/tremor codec/libfaad2
	
.c.o:
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<
	
i2s.o:output/i2s.c
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<

rlog.o:rlog/rlog.c
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<
	
ffmpeg.o:codec/ffmpeg.c
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<

tremor.o:codec/tremor.c
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<
