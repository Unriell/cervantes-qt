#!/bin/bash

# Instructions for qt-everywhere-opensource-src-4.8.1
#
# We do not set CXX or CC, and then bootstap moc and qmake are build with those and compilation fails
# We need to set links to arm-linux-gnueabi-g++-4.6 and arm-linux-gnueabi-gcc-4.6 name arm-linux-g++ and arm-linux-gcc. Same for -as

# We need to remove previous versions of Qt from bootstrap as well as libpng.a coming from adobe package

# To get the dbus test working you need to add these two lines to ./config.tests/unix/dbus/dbus.pro:
#  QMAKE_CXXFLAGS = -I/opt/mx508-rootfs-devel/usr/include -I/opt/mx508-rootfs-devel/usr/include/dbus-1.0 -I/opt/mx508-rootfs-devel/usr/lib/arm-linux-gnueabi/dbus-1.0/include
#  LIBS = -L/opt/mx508-rootfs-devel/usr/lib/arm-linux-gnueabi/ -ldbus-1 -lpthread -lrt
# Finally for ./src/dbus/dbus.pro:
#  QMAKE_CXXFLAGS = -I/opt/mx508-rootfs-devel/usr/include -I/opt/mx508-rootfs-devel/usr/include/dbus-1.0 -I/opt/mx508-rootfs-devel/usr/lib/arm-linux-gnueabi/dbus-1.0/include

# Notice that doing "native" compilation with qemu results on weird webkit bugs calling QNetworkCookieJar.
# We need to do cross compiling. However when cross compiling
# the jpeg plugin (even using qt own libjpeg) fails to open jpgs. So the only work around to get a "good"
# qt build is to do a cross compilation and then replace the libqjpeg.so plugin binary with one generated
# on a native build



export FSROOT=
export QT_CFLAGS_DBUS="-I$FSROOT/usr/include -I$FSROOT/usr/include/dbus-1.0 -I$FSROOT/usr/lib/arm-linux-gnueabi/dbus-1.0/include"
export QT_LIBS_DBUS="-L$FSROOT/usr/lib/arm-linux-gnueabi/ -ldbus"
#export CXXFLAGS="-march=armv7"
export CXX=arm-linux-gnueabi-g++-4.6
export CC=arm-linux-gnueabi-gcc-4.6

make confclean

./configure -prefix /usr \
-opensource \
-qt-libmng \
-qt-sql-sqlite \
-no-javascript-jit \
-no-accessibility \
-embedded arm \
-fontconfig \
-release \
-no-opengl \
-little-endian \
-nomake examples \
-confirm-license \
-nomake demos \
-plugin-gfx-einkfb \
-plugin-mouse-tslib \
-plugin-kbd-linuxinput \
-plugin-kbd-eb600keypad \
-dbus \
-script \
-no-scripttools \
-no-qt3support \
-system-libpng \
-qt-libjpeg \
-system-zlib \
-verbose \
-I$FSROOT/usr/include \
-I$FSROOT/usr/include/arm-linux-gnueabi \
-L$FSROOT/usr/lib \
-L$FSROOT/usr/lib/arm-linux-gnueabi/ \
-L$FSROOT/lib/arm-linux-gnueabi/ -lpng -lz -lrt \
-force-pkg-config \
-D QT_KEYPAD_NAVIGATION

MAKEFLAGS='-j8' make
