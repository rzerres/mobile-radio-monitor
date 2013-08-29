
noinst_PROGRAMS = test-graph

test_graph_SOURCES = \
	$(top_srcdir)/src/mrm-graph.h $(top_srcdir)/src/mrm-graph.c \
	test-graph.c

test_graph_CPPFLAGS = \
	$(MM_CFLAGS) \
	$(GTK_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src

test_graph_LDADD = \
	$(MM_LIBS) \
	$(GTK_LIBS) \
	-lm
