drvdir=$(libdir)/xorg/modules/drivers
drv_LTLIBRARIES=librpi.la
librpi_la_SOURCES=rpi_video.c
librpi_la_CFLAGS = -std=gnu99 $(XORG_CFLAGS) $(GL_CFLAGS)
librpi_la_LDFLAGS = -module -avoid-version $(XORG_LDFLAGS) $(GL_LDFLAGS)
