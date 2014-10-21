bin_PROGRAMS = gmediarender
gmediarender_SOURCES = main.c \
	upnp.c upnp_control.c upnp_connmgr.c  upnp_transport.c \
	upnp.h upnp_control.h upnp_connmgr.h  upnp_transport.h \
	upnp_device.c upnp_device.h \
	upnp_renderer.h upnp_renderer.c \
	webserver.c webserver.h \
	output_gstreamer.c  output_gstreamer.h \
	logging.h \
	debug.c rlog.c rlog.h \
	xmlescape.c xmlescape.h \
	uuid.c uuid.h

AM_LDFLAGS = $(UPNP_LDFLAGS)
AM_CPPFLAGS = $(GST_CFLAGS) $(UPNP_CPPFLAGS) -DPKG_DATADIR=\"$(datadir)/gmediarender\"
gmediarender_LDADD = $(GST_LIBS) $(UPNP_LIBS) 
