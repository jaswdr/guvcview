## Process this file with automake to produce Makefile.in

SUBDIRS = gview_v4l2core \
		  gview_audio \
		  gview_render \
		  gview_encoder \
          guvcview \
          data \
          po \
          po/gview_v4l2core

#Distribute these directories:
DIST_SUBDIRS =  gview_v4l2core \
				gview_audio \
				gview_render \
				gview_encoder \
				guvcview \
				data \
				po \
				po/gview_v4l2core

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/libgviewv4l2core.pc \
				 pkgconfig/libgviewaudio.pc \
				 pkgconfig/libgviewrender.pc \
				 pkgconfig/libgviewencoder.pc

ACLOCAL_AMFLAGS = -I m4

docdir = ${datadir}/doc/guvcview
doc_DATA = \
	README.md\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL

EXTRA_DIST = bootstrap.sh

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then echo "Missing gettext. Rerun configure and check for" \
	"'checking whether to use NLS... yes'!" ; exit 1 ; fi

update-po: check-gettext
	cd $(srcdir); \
	echo "#desktop file" >> po/POTFILES.in.2; \
	echo "data/guvcview.desktop.in.in" >> po/POTFILES.in.2; \
	echo "#appdata file" >> po/POTFILES.in.2; \
	echo "data/guvcview.appdata.xml.in.in" >> po/POTFILES.in.2; \
	echo "#gview_v4l2core lib" >> po/POTFILES.in.2; \
	grep '_(' `find gview_v4l2core/ -name "*c"` | cut -d: -f1 | sort -u >> po/POTFILES.in.2 ; \
	echo "#gview_render lib" >> po/POTFILES.in.2; \
	grep '_(' `find gview_render/ -name "*c"` | cut -d: -f1 | sort -u >> po/POTFILES.in.2 ; \
	echo "#gview_audio lib" >> po/POTFILES.in.2; \
	grep '_(' `find gview_audio/ -name "*c"` | cut -d: -f1 | sort -u >> po/POTFILES.in.2 ; \
	echo "#gview_encoder" >> po/POTFILES.in.2; \
	grep '_(' `find gview_encoder/ -name "*c"` | cut -d: -f1 | sort -u >> po/POTFILES.in.2 ; \
	echo "#guvcview" >> po/POTFILES.in.2; \
	grep '_(' `find guvcview/ -name "*c"` | cut -d: -f1 | sort -u >> po/POTFILES.in.2 ; \
	if diff po/POTFILES.in po/POTFILES.in.2 >/dev/null 2>&1 ; then \
		rm -f po/POTFILES.in.2 ; \
	else \
		mv po/POTFILES.in.2 po/POTFILES.in ; \
	fi
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po
	cd $(srcdir); \
	echo "#gview_v4l2core lib" >> po/gview_v4l2core/POTFILES.in.2; \
	grep '_(' `find gview_v4l2core/ -name "*c"` | cut -d: -f1 | sort -u >> po/gview_v4l2core/POTFILES.in.2 ; \
	if diff po/gview_v4l2core/POTFILES.in po/gview_v4l2core/POTFILES.in.2 >/dev/null 2>&1 ; then \
		rm -f po/gview_v4l2core/POTFILES.in.2 ; \
	else \
		mv po/gview_v4l2core/POTFILES.in.2 po/gview_v4l2core/POTFILES.in ; \
	fi
	cd po/gview_v4l2core && $(MAKE) $(AM_MAKEFLAGS) update-po

update-gmo: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo: check-gettext
	touch $(srcdir)/po/*.po
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

.PHONY: check-gettext update-po update-gmo force-update-gmo
