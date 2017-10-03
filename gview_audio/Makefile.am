## Process this file with automake to produce Makefile.in

# Public headers
h_sources = gviewaudio.h

c_sources = audio.c \
			audio_fx.c \
			core_time.c \
			audio_portaudio.c

if HAVE_PULSEAUDIO
c_sources += audio_pulseaudio.c
endif


#Install the headers in a versioned directory - guvcvideo-x.x/libgviewaudio:
library_includedir=$(includedir)/$(PACKAGE)-$(PACKAGE_RELEASE)/$(GVIEWAUDIO_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

#AM_CFLAGS =

lib_LTLIBRARIES= libgviewaudio.la

libgviewaudio_la_SOURCES= $(h_sources) $(c_sources)

libgviewaudio_la_CFLAGS = $(GVIEWAUDIO_CFLAGS) \
			$(PULSE_CFLAGS) \
			$(PTHREAD_CFLAGS) \
			-I$(top_srcdir) \
			-I$(top_srcdir)/includes

libgviewaudio_la_LIBADD= $(GVIEWAUDIO_LIBS) $(PULSE_LIBS) $(PTHREAD_LIBS)

libgviewaudio_la_LDFLAGS= -version-info $(GVIEWAUDIO_LIBRARY_VERSION) -release $(GVIEWAUDIO_API_VERSION)


