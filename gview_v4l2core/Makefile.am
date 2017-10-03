## Process this file with automake to produce Makefile.in

# Public headers
h_sources = gviewv4l2core.h \
			$(top_srcdir)/includes/gview.h

c_sources = v4l2_core.c \
			v4l2_formats.c \
			v4l2_controls.c \
			v4l2_devices.c \
			v4l2_xu_ctrls.c \
			uvc_h264.c \
			core_time.c \
			frame_decoder.c \
			colorspaces.c \
			jpeg_decoder.c \
			soft_autofocus.c \
			dct.c \
			control_profile.c \
			save_image.c \
			save_image_jpeg.c \
			save_image_bmp.c \
			save_image_png.c


#Install the headers in a versioned directory - guvcvideo-x/libgviewv4l2core:
library_includedir=$(includedir)/$(PACKAGE)-$(PACKAGE_RELEASE)/$(GVIEWV4L2CORE_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

#AM_CFLAGS =

lib_LTLIBRARIES= libgviewv4l2core.la

libgviewv4l2core_la_SOURCES= $(h_sources) $(c_sources)

libgviewv4l2core_la_CFLAGS = $(GVIEWV4L2CORE_CFLAGS) \
			$(PTHREAD_CFLAGS) \
			-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
			-I$(top_srcdir) \
			-I$(top_srcdir)/includes

libgviewv4l2core_la_LIBADD= $(GVIEWV4L2CORE_LIBS) $(PTHREAD_LIBS) -lm

libgviewv4l2core_la_LDFLAGS= -version-info $(GVIEWV4L2CORE_LIBRARY_VERSION) -release $(GVIEWV4L2CORE_API_VERSION)


