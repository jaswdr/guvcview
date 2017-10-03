SUBDIRS = icons
@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_XML_RULE@

dist_man_MANS = guvcview.1

if DEBIAN_MENU

MENU_IN_FILES= guvcview.in
MENU_FILES= guvcview

$(MENU_FILES): $(MENU_IN_FILES)
	cp $? $@

menudir = ${datadir}/menu
menu_DATA = $(MENU_FILES)

endif

if DESKTOP

DESKTOP_IN_FILES= guvcview.desktop.in
DESKTOP_FILES= $(DESKTOP_IN_FILES:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(DESKTOP_FILES)

APPDATA_IN_FILES= guvcview.appdata.xml.in
APPDATA_FILES= $(APPDATA_IN_FILES:.xml.in=.xml)

appdatadir = $(datadir)/appdata
appdata_DATA = $(APPDATA_FILES)

endif

CLEANFILES = \
		$(menu_DATA)\
		$(desktop_DATA) \
		$(appdata_DATA)

EXTRA_DIST = \
		$(APPDATA_IN_FILES) \
		$(DESKTOP_IN_FILES)\
		$(MENU_IN_FILES)

clean-generic:
	test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES); \
	test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)
