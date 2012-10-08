#!/bin/sh
export CFLAGS="-O0 -g3"
./configure --prefix=/usr --localstatedir=/var  --sysconfdir=/etc --disable-dmx --disable-xvfb --disable-xnest --disable-xquartz --disable-xwin --disable-kdrive --disable-xfake --disable-xfbdev --disable-xdirectfb
