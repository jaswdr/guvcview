## Process this file with automake to produce Makefile.in

# Public headers
h_sources = gviewencoder.h

c_sources = encoder.c \
			video_codecs.c \
			audio_codecs.c \
			libav_encoder.c \
			stream_io.c \
			file_io.c \
			matroska.c \
			avi.c \
			muxer.c


#Install the headers in a versioned directory - guvcvideo-x.x/libgviewaudio:
library_includedir=$(includedir)/$(PACKAGE)-$(PACKAGE_RELEASE)/$(GVIEWENCODER_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

#AM_CFLAGS =

lib_LTLIBRARIES= libgviewencoder.la

libgviewencoder_la_SOURCES= $(h_sources) $(c_sources)

libgviewencoder_la_CFLAGS = $(GVIEWENCODER_CFLAGS) \
			$(PTHREAD_CFLAGS) \
			-I$(top_srcdir) \
			-I$(top_srcdir)/includes

libgviewencoder_la_LIBADD= $(GVIEWENCODER_LIBS) $(GSL_LIBS) $(PTHREAD_LIBS) -lm

libgviewencoder_la_LDFLAGS= -version-info $(GVIEWENCODER_LIBRARY_VERSION) -release $(GVIEWENCODER_API_VERSION)
