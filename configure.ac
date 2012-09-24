#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xf86-video-rpi], [0.1], [adrian.cheater@gmail.com])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([src/rpi_video.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

# Libtool stuff
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([XORG],[xorg-server])
PKG_CHECK_MODULES([GL],[egl glesv2 bcm_host])
# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
