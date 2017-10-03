## Process this file with automake to produce Makefile.in

#AM_CFLAGS =

bin_PROGRAMS = guvcview

guvcview_SOURCES = guvcview.c \
				   video_capture.c \
				   core_io.c \
				   options.c \
				   config.c \
				   gui.c

if HAVE_GTK3
guvcview_SOURCES += gui_gtk3.c \
				   gui_gtk3_menu.c \
				   gui_gtk3_v4l2ctrls.c \
				   gui_gtk3_h264ctrls.c \
				   gui_gtk3_videoctrls.c \
				   gui_gtk3_audioctrls.c \
				   gui_gtk3_callbacks.c
endif

if HAVE_QT5
guvcview_SOURCES += moc_gui_qt5.cpp \
				   gui_qt5.cpp \
				   gui_qt5_menu.cpp \
				   gui_qt5_v4l2ctrls.cpp \
				   gui_qt5_h264ctrls.cpp \
				   gui_qt5_videoctrls.cpp \
				   gui_qt5_audioctrls.cpp \
				   gui_qt5_callbacks.cpp
endif

guvcview_CFLAGS = $(PTHREAD_CFLAGS) \
		  -D_REENTRANT\
		  -D_FILE_OFFSET_BITS=64\
		  -Wall\
		  -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
		  -DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
		  -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
		  $(DEPS_CFLAGS) $(EXTRA_CFLAGS) $(PTHREAD_CFLAGS) \
		  $(EXTRA_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/includes \
		  -I$(top_srcdir)/gview_v4l2core \
		  -I$(top_srcdir)/gview_render \
		  -I$(top_srcdir)/gview_audio \
		  -I$(top_srcdir)/gview_encoder

if HAVE_GTK3
guvcview_CFLAGS += $(GUIGTK3_CFLAGS)
endif

if HAVE_QT5
guvcview_CPPFLAGS = $(guvcview_CFLAGS) \
					$(GUIQT5_CFLAGS) \
					-fPIC
endif		  

guvcview_LDFLAGS = $(LIBINTL)

guvcview_LDADD = ../gview_v4l2core/$(GVIEWV4L2CORE_LIBRARY_NAME).la \
				../gview_render/$(GVIEWRENDER_LIBRARY_NAME).la \
				../gview_audio/$(GVIEWAUDIO_LIBRARY_NAME).la \
				../gview_encoder/$(GVIEWENCODER_LIBRARY_NAME).la \
				$(PTHREAD_LIBS) \
				-lm
if HAVE_GTK3
guvcview_LDADD += $(GUIGTK3_LIBS)
endif

if HAVE_QT5
guvcview_LDADD += $(GUIQT5_LIBS)
endif

moc_%.cpp: %.hpp
	$(MOC) $< -o $@

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@



