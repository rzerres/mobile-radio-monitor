AC_PREREQ([2.64])

AC_INIT([mobile-radio-monitor], [1.0], [aleksander@aleksander.es], [mobile-radio-monitor])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src/mrm-main.c)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall foreign no-define tar-ustar no-dist-gzip dist-xz -Wno-portability])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

PKG_PROG_PKG_CONFIG

GNOME_COMPILE_WARNINGS([maximum])

AC_CHECK_LIBM
AC_SUBST(LIBM)

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.9.8)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(QMI, qmi-glib >= 1.4)
AC_SUBST(QMI_CFLAGS)
AC_SUBST(QMI_LIBS)

PKG_CHECK_MODULES(GUDEV, gudev-1.0 >= 147)
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
AC_SUBST(GLIB_COMPILE_RESOURCES)

AC_CONFIG_FILES([
Makefile
build-aux/Makefile
data/Makefile
src/Makefile
tests/Makefile
])

AC_OUTPUT

echo "
    mobile-radio-monitor $VERSION
    ==============================================

    compiler: ${CC}
    cflags:   ${CFLAGS}
"
