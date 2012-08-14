#!/bin/sh

gcc -I/usr/include/xorg -I/usr/include/pixman-1 -c rpi_video.c -o rpi_video.o
gcc -shared -W1,-soname,librpi.so -o librpi.so rpi_video.o
sudo cp librpi.so /usr/local/lib/xorg/modules 
