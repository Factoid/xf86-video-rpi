#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xf86-video-rpi], [0.1], [adrian.cheater@gmail.com])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

XORG_MACROS_VERSION([1.8])
XORG_DEFAULT_OPTIONS

# Libtool stuff
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Checks for libraries.
PKG_CHECK_MODULES([XORG],[xorg-server])
PKG_CHECK_MODULES([GL],[egl glesv2 bcm_host])

# Checks for header files.
drvdir=$libdir/xorg/modules/drivers
AC_SUBST([drvdir])
AC_SUBST([moduledir])
DRIVER_NAME=fbdev
AC_SUBST([DRIVER_NAME])
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
