#!/bin/sh
cd src
gcc -g -std=gnu99 -I. -I.. -I/usr/include/xorg -I/usr/include/pixman-1 `pkg-config --cflags xorg-server egl glesv2 bcm_host` -c rpi_video.c -o rpi_video.o
gcc -g `pkg-config --libs xorg-server egl glesv2 bcm_host` -shared -W1,-soname,librpi.so -o librpi.so rpi_video.o
sudo cp librpi.so /usr/local/lib/xorg/modules/drivers
cd ../
sudo cp ./config/*.conf /usr/share/X11/xorg.conf.d/
