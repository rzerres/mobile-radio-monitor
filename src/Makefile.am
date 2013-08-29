
################################################################################
# Program

bin_PROGRAMS = mobile-radio-monitor

mobile_radio_monitor_SOURCES = \
	mrm-resources.h mrm-resources.c \
	mrm-graph.h mrm-graph.c \
	mrm-app.h mrm-app.c \
	mrm-main.c

mobile_radio_monitor_CPPFLAGS = \
	$(MM_CFLAGS) \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)

mobile_radio_monitor_LDADD = \
	$(MM_LIBS) \
	$(GTK_LIBS) \
	-lm

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

CLEANFILES = mrm-resources.h mrm-resources.c
