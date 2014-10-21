SUBDIRS=lib src
BINPATH=bin
DMPLAY_PATH=$(shell pwd)

include $(DMPLAY_PATH)/config.mak

all:
	@set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i; \
	done

install:all
	cp -f src/dmplay $(BINPATH)
ifeq ("$(CONFIG_FFMPEG)", "yes")	
	cp -fr src/codec/ffmpeg/lib $(BINPATH)
	rm -f $(BINPATH)/lib/libswscale* $(BINPATH)/lib/libavdevice*
endif
ifeq ("$(CONFIG_FFMPEG_AAC_DECODER_FIXED)", "yes")
	cp -fr src/codec/libfaad2/lib $(BINPATH)
else
ifeq ("$(CONFIG_FAAD2)", "yes")
	cp -fr src/codec/libfaad2/lib $(BINPATH)
endif
endif
ifeq ("$(CONFIG_FFMPEG_OGG_DECODER_FIXED)", "yes")
	cp -fr src/codec/libogg/lib $(BINPATH)
	cp -fr src/codec/tremor/lib $(BINPATH)
else
ifeq ("$(CONFIG_TREMOR)", "yes")
	cp -fr src/codec/libogg/lib $(BINPATH)
	cp -fr src/codec/tremor/lib $(BINPATH)
endif
ifeq ("$(CONFIG_LIBOGG)", "yes")
	cp -fr src/codec/libogg/lib $(BINPATH)
endif
endif

clean:
	@set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i clean; \
	done

distclean:
	@set -e; \
	for i in $(SUBDIRS); do \
		$(MAKE) -C $$i distclean; \
	done