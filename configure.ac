AC_PREREQ([2.64])

AC_INIT([mobile-radio-monitor], [0.1], [aleksander@gnu.org], [mobile-radio-monitor])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR(src/mrm-main.c)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall foreign no-define tar-ustar no-dist-gzip dist-xz])

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

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.8.2)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(MM, mm-glib >= 1.0)
AC_SUBST(MM_CFLAGS)
AC_SUBST(MM_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT

echo "
    mobile-radio-monitor $VERSION
    ==============================================

    compiler:                ${CC}
    cflags:                  ${CFLAGS}
"
