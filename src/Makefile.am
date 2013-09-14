
################################################################################
# Program

bin_PROGRAMS = mobile-radio-monitor

mobile_radio_monitor_SOURCES = \
	mrm-resources.h mrm-resources.c \
	mrm-error.h mrm-error-types.h mrm-error-types.c \
	mrm-enum-types.h mrm-enum-types.c \
	mrm-color-icon.h mrm-color-icon.c \
	mrm-graph.h mrm-graph.c \
	mrm-device.h mrm-device.c \
	mrm-signal-tab.h mrm-signal-tab.c \
	mrm-window.h mrm-window.c \
	mrm-app.h mrm-app.c \
	mrm-main.c

mobile_radio_monitor_CPPFLAGS = \
	$(QMI_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GUDEV_CFLAGS) \
	-I$(top_srcdir)

mobile_radio_monitor_LDADD = \
	$(QMI_LIBS) \
	$(GTK_LIBS) \
	$(GUDEV_LIBS) \
	-lm

################################################################################
# Error types

mrm-error-types.h: mrm-error.h $(top_srcdir)/build-aux/mrm-error-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#ifndef __MRM_ERROR_TYPES_H__\n#define __MRM_ERROR_TYPES_H__\n" \
		--template $(top_srcdir)/build-aux/mrm-error-types-template.h \
		--ftail "#endif /* __MRM_ERROR_TYPES_H__ */\n" \
		mrm-error.h > $@

mrm-error-types.c: Makefile.am mrm-error.h $(top_srcdir)/build-aux/mrm-error-types-template.c mrm-error-types.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mrm-error.h\"\n#include \"mrm-error-types.h\"\n" \
		--template $(top_srcdir)/build-aux/mrm-error-types-template.c \
		mrm-error.h > $@

################################################################################
# Enum types

ENUMS = \
	$(srcdir)/mrm-device.h \
	$(srcdir)/mrm-graph.h

mrm-enum-types.h: Makefile.am $(ENUMS) $(top_srcdir)/build-aux/mrm-enum-types-template.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mrm-device.h\"\n#include \"mrm-graph.h\"\n#ifndef __MRM_ENUM_TYPES_H__\n#define __MRM_ENUM_TYPES_H__\n" \
		--template $(top_srcdir)/build-aux/mrm-enum-types-template.h \
		--ftail "#endif /* __MRM_ENUM_TYPES_H__ */\n" \
		$(ENUMS) > $@

mrm-enum-types.c: Makefile.am $(top_srcdir)/build-aux/mrm-enum-types-template.c mrm-enum-types.h
	$(AM_V_GEN) $(GLIB_MKENUMS) \
		--fhead "#include \"mrm-enum-types.h\"" \
		--template $(top_srcdir)/build-aux/mrm-enum-types-template.c \
		$(ENUMS) > $@

################################################################################
# Resource files

resource_file_dependencies = \
	$(shell $(GLIB_COMPILE_RESOURCES) \
	--sourcedir=$(srcdir) \
	--generate-dependencies \
	$(srcdir)/mrm.gresource.xml)

mrm-resources.c: mrm.gresource.xml $(resource_file_dependencies)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
		--target=$@ \
		--sourcedir=$(srcdir) \
		--generate-source \
		--c-name mrm \
		$(srcdir)/mrm.gresource.xml

mrm-resources.h: mrm.gresource.xml $(resource_file_dependencies)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) \
		--target=$@ \
		--sourcedir=$(srcdir) \
		--generate-header \
		--c-name mrm \
		$(srcdir)/mrm.gresource.xml

################################################################################

CLEANFILES = \
	mrm-resources.h mrm-resources.c \
	mrm-error-types.h mrm-error-types.c \
	mrm-enum-types.h mrm-enum-types.c

EXTRA_DIST = \
	mrm-window.ui \
	mrm-signal-tab.ui
