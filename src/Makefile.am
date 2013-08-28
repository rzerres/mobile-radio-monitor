
bin_PROGRAMS = mobile-radio-monitor

mobile_radio_monitor_SOURCES = \
	mrm-graph.h mrm-graph.c \
	mrm-main.c

mobile_radio_monitor_CPPFLAGS = \
	$(MM_CFLAGS) \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)

mobile_radio_monitor_LDADD = \
	$(MM_LIBS) \
	$(GTK_LIBS) \
	-lm
