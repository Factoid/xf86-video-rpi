#!/bin/sh
cd src
gcc -g -I/usr/include/xorg -I/usr/include/pixman-1 -c rpi_video.c -o rpi_video.o
gcc -g -shared -W1,-soname,librpi.so -o librpi.so rpi_video.o
sudo cp librpi.so /usr/local/lib/xorg/modules 
cd ../
sudo cp ./config/*.conf /usr/share/X11/xorg.conf.d/
