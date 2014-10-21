SUBDIRS=mrender dmplay
DMR_INSTALL_PATH=rootfs
DMR_PATH=$(shell pwd)

all:
	@set -e; \
	for i in $(SUBDIRS); do \
			$(MAKE) -C $$i; \
	done

install:
	@set -e; \
	for i in $(SUBDIRS); do \
			$(MAKE) install -C $$i; \
	done
	-mkdir -p $(DMR_INSTALL_PATH)/bin $(DMR_INSTALL_PATH)/lib
	cp -f mrender/bin/gmediarender $(DMR_INSTALL_PATH)/bin
	cp -fr mrender/bin/lib $(DMR_INSTALL_PATH)
	cp -f dmplay/bin/dmplay $(DMR_INSTALL_PATH)/bin
	cp -fr dmplay/bin/lib $(DMR_INSTALL_PATH)

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