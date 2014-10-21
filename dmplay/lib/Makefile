CROSS_PREFIX = mips-linux-
STRIP = $(CROSS_PREFIX)strip
LIB_PATH = $(shell pwd)
include $(LIB_PATH)/../config.mak
LIB_DEPENDS = 

# faad2
FAAD2_SRC_PATH = $(LIB_PATH)/faad2-2.7
FAAD2_INSTALL_PATH = $(LIB_PATH)/../src/codec/libfaad2
FAAD2_CONFIGURE = ./configure  --host=mips-linux --prefix=$(FAAD2_INSTALL_PATH) 
FFMPEG_EXTRA_LIBFAAD = -L$(FAAD2_INSTALL_PATH)/lib -lfaad

# libogg
LIBOGG_SRC_PATH = $(LIB_PATH)/libogg-1.3.2
LIBOGG_INSTALL_PATH=$(LIB_PATH)/../src/codec/libogg
LIBOGG_CONFIGURE = ./configure  --host=mips-linux --prefix=$(LIBOGG_INSTALL_PATH) 
FFMPEG_EXTRA_LIBOGG = -L$(LIBOGG_INSTALL_PATH)/lib -logg

# tremor
TREMOR_SRC_PATH = $(LIB_PATH)/tremor
TREMOR_INSTALL_PATH = $(LIB_PATH)/../src/codec/tremor
TREMOR_CONFIGURE = ./configure  --host=mips-linux --prefix=$(TREMOR_INSTALL_PATH) CFLAGS='-I$(LIBOGG_INSTALL_PATH)/include' OGG_LIBS='-L$(LIBOGG_INSTALL_PATH)/lib -logg'
FFMPEG_EXTRA_TREMOR = -L$(TREMOR_INSTALL_PATH)/lib -lvorbisidec

# depend libs
ifeq ("$(CONFIG_FAAD2)", "yes")
LIB_DEPENDS += faad2
endif
ifeq ("$(CONFIG_TREMOR)", "yes")
LIB_DEPENDS += libtremor
else
ifeq ("$(CONFIG_LIBOGG)", "yes")
LIB_DEPENDS += libogg
endif
endif

# ffmpeg extra libs && depends && decoder
FFMPEG_EXTRA_LIBS = 
FFMPEG_AAC = aac
FFMPEG_OGG = vorbis
FFMPEG_WMA = wmav1,wmav2
ifeq ("$(CONFIG_FFMPEG_AAC_DECODER_FIXED)", "yes")
FFMPEG_EXTRA_LIBS += $(FFMPEG_EXTRA_LIBFAAD)
FFMPEG_AAC = aac_fixed
FFMPEG_DEPENDS += faad2
endif
ifeq ("$(CONFIG_FFMPEG_OGG_DECODER_FIXED)", "yes")
FFMPEG_EXTRA_LIBS += $(FFMPEG_EXTRA_LIBOGG) $(FFMPEG_EXTRA_TREMOR)
FFMPEG_OGG = vorbis_fixed
FFMPEG_DEPENDS += libtremor
endif
ifeq ("$(CONFIG_FFMPEG_WMA_DECODER_FIXED)", "yes")
FFMPEG_WMA = wmav1_fixed,wmav2_fixed
endif 

# ffmpeg
FFMPEG_SRC_PATH = $(LIB_PATH)/ffmpeg-0.7.16
FFMPEG_INSTALL_PATH = $(LIB_PATH)/../src/codec/ffmpeg
FFMPEG_ENABLE_PROTOCOL = file,http,tcp,udp
FFMPEG_ENABLE_DEMUXER = asf,mp3,pcm_s16be,pcm_s16le,pcm_s8,pcm_u8,pcm_s24le,pcm_s32le,aac,ogg,flac,ape,wav,mov
FFMPEG_ENABLE_DECODER = $(FFMPEG_WMA),$(FFMPEG_AAC),mp2,mp3,pcm_s16be,pcm_s16le,pcm_s8,pcm_u8,pcm_s24le,pcm_s32le,flac,ape,$(FFMPEG_OGG),wavpack,adpcm_ima_wav
FFMPEG_ENABLE_PARSER = mpegaudio,aac

FFMPEG_CONFIGURE = ./configure --extra-libs='$(FFMPEG_EXTRA_LIBS)' --enable-cross-compile --cross-prefix=$(CROSS_PREFIX) --arch=mips --target-os=linux --prefix=$(FFMPEG_INSTALL_PATH) --enable-shared --enable-static --disable-debug --enable-gpl --enable-pthreads --disable-optimizations --enable-small --disable-stripping --enable-zlib --disable-bsfs --disable-devices --disable-encoders --disable-decoders --disable-muxers --disable-demuxers --disable-parsers --disable-protocols --enable-network --disable-yasm --enable-protocol='$(FFMPEG_ENABLE_PROTOCOL)' --enable-demuxer='$(FFMPEG_ENABLE_DEMUXER)' --enable-decoder='$(FFMPEG_ENABLE_DECODER)' --enable-parser='$(FFMPEG_ENABLE_PARSER)'

# depend libs
ifeq ("$(CONFIG_FFMPEG)", "yes")
LIB_DEPENDS += ffmpeg
endif

# rules
all: $(LIB_DEPENDS)

faad2:
	cd $(FAAD2_SRC_PATH) && $(FAAD2_CONFIGURE) && make && make install
	rm -fr $(FAAD2_INSTALL_PATH)/lib/*.a $(FAAD2_INSTALL_PATH)/lib/*.la
	$(STRIP) $(FAAD2_INSTALL_PATH)/lib/*

libogg:
	cd $(LIBOGG_SRC_PATH) && $(LIBOGG_CONFIGURE) && make && make install
	rm -fr $(LIBOGG_INSTALL_PATH)/lib/*.a $(LIBOGG_INSTALL_PATH)/lib/*.la $(LIBOGG_INSTALL_PATH)/lib/pkgconfig
	$(STRIP) $(LIBOGG_INSTALL_PATH)/lib/*

libtremor: libogg
	cd $(TREMOR_SRC_PATH) && $(TREMOR_CONFIGURE) && make && make install
	rm -fr $(TREMOR_INSTALL_PATH)/lib/*.a $(TREMOR_INSTALL_PATH)/lib/*.la $(TREMOR_INSTALL_PATH)/lib/pkgconfig
	$(STRIP) $(TREMOR_INSTALL_PATH)/lib/*
	
ffmpeg: $(FFMPEG_DEPENDS)
	cd $(FFMPEG_SRC_PATH) && $(FFMPEG_CONFIGURE) && make && make install
	rm -fr $(FFMPEG_INSTALL_PATH)/lib/*.a $(FFMPEG_INSTALL_PATH)/lib/pkgconfig	
	$(STRIP) $(FFMPEG_INSTALL_PATH)/lib/*

clean:
	cd $(FFMPEG_SRC_PATH) && make clean
	cd $(TREMOR_SRC_PATH) && make clean
	cd $(LIBOGG_SRC_PATH) && make clean
	cd $(FAAD2_SRC_PATH) && make clean

distclean:
	cd $(FFMPEG_SRC_PATH) && make distclean
	cd $(TREMOR_SRC_PATH) && make distclean
	cd $(LIBOGG_SRC_PATH) && make distclean
	cd $(FAAD2_SRC_PATH) && make distclean
