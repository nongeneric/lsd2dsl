#!/bin/bash

# point LINUXDEPLOY to linuxdeploy-x86_64.AppImage
# put linuxdeploy-plugin-qt-x86_64.AppImage next to it
# point SRC to the source directory

set -e

rm -rf AppDir
rm -f *.AppImage

ninja lsd2dsl
${LINUXDEPLOY} --plugin qt \
    --appdir=AppDir \
    -d ${SRC}/scripts/appimage/lsd2dsl.desktop \
    -e app/lsd2dsl \
    -i ${SRC}/scripts/appimage/lsd2dsl.svg

cp /usr/local/resources/v8_context_snapshot.bin ./AppDir/usr/resources

${LINUXDEPLOY} \
    --appdir=AppDir \
    --output appimage

LIBC=`/usr/lib64/libc.so.6 | head -n1 | perl -pe 's/.*?((\d+\.\d+)+).*/$1/g'`
VERSION=`grep "project(" -i $SRC/CMakeLists.txt | awk -F " " '{print $3}'`
mv lsd2dsl-x86_64.AppImage lsd2dsl-${VERSION}.x86_64.glibc_${LIBC}.AppImage
