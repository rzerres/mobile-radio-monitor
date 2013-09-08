
noinst_PROGRAMS = test-graph

test_graph_SOURCES = \
	$(top_srcdir)/src/mrm-color-icon.h $(top_srcdir)/src/mrm-color-icon.c \
	$(top_srcdir)/src/mrm-graph.h $(top_srcdir)/src/mrm-graph.c \
	test-graph.c

test_graph_CPPFLAGS = \
	$(QMI_CFLAGS) \
	$(GTK_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

test_graph_LDADD = \
	$(QMI_LIBS) \
	$(GTK_LIBS) \
	-lm
