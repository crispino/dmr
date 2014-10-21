CROSS_PREFIX = mips-linux-
STRIP = $(CROSS_PREFIX)strip
MRENDER_PATH = $(shell pwd)
BINPATH = bin

# libupnp
LIBUPNP_SRC_PATH = $(MRENDER_PATH)/libupnp-1.6.17
LIBUPNP_INSTALL_PATH=$(MRENDER_PATH)/gmediarender/libupnp
LIBUPNP_CONFIGURE = ./configure --host=mips-linux --build=i686-linux --prefix=$(LIBUPNP_INSTALL_PATH) --disable-blocking_tcp_connections

# gmediarender
GMEDIARENDER_SRC_PATH = $(MRENDER_PATH)/gmediarender
GMEDIARENDER_INSTALL_PATH=$(MRENDER_PATH)/gmediarender/src
EXTERN_CPPFLAGS	= -I$(LIBUPNP_INSTALL_PATH)/include
EXTERN_LDFLAGS	= '-L$(LIBUPNP_INSTALL_PATH)/lib -lupnp -lixml -lthreadutil -lpthread'
GMEDIARENDER_CONFIGURE = ./configure --host=mips-linux --no-recursion CPPFLAGS=$(EXTERN_CPPFLAGS) LDFLAGS=$(EXTERN_LDFLAGS)

all: libupnp gmrender

install: all
	cp -fr $(LIBUPNP_INSTALL_PATH)/lib $(BINPATH)
	cp -fr $(GMEDIARENDER_INSTALL_PATH)/gmediarender $(BINPATH)

libupnp:
	cd $(LIBUPNP_SRC_PATH) && $(LIBUPNP_CONFIGURE) && make && make install
	rm -fr $(LIBUPNP_INSTALL_PATH)/lib/*.a $(LIBUPNP_INSTALL_PATH)/lib/*.la $(LIBUPNP_INSTALL_PATH)/lib/pkgconfig
	$(STRIP) $(LIBUPNP_INSTALL_PATH)/lib/*
	
gmrender:
	cd $(GMEDIARENDER_SRC_PATH) && $(GMEDIARENDER_CONFIGURE) && make && make install
	$(STRIP) $(GMEDIARENDER_INSTALL_PATH)/gmediarender
	
clean:
	cd $(LIBUPNP_SRC_PATH) && make clean
	cd $(GMEDIARENDER_SRC_PATH) && make clean

distclean:
	cd $(LIBUPNP_SRC_PATH) && make distclean
	cd $(GMEDIARENDER_SRC_PATH) && make distclean	
