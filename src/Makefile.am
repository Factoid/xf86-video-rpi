drvdir=$(libdir)/xorg/modules/drivers
drv_LTLIBRARIES=librpi.la
AM_CFLAGS=@XORG_CFLAGS@ @GL_CFLAGS@
AM_LDFLAGS=@XORG_LIBS@ @GL_LIBS@
librpi_la_SOURCES=rpi_video.c
librpi_la_CFLAGS=@XORG_CFLAGS@ @GL_CFLAGS@
librpi_la_LDFLAGS=-module -avoid-version @XORG_LIBS@ @GL_LIBS@
