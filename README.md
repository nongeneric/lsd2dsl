lsd2dsl
=======

ABBYY Lingvo dictionaries decompiler. Supports the Lingvo x5 and x6 formats.

## Building on Fedora 20
Install required libraries

    yum install libzip-devel boost-devel cmake qt5-qtbase-devel libvorbis-devel libsndfile-devel
    
Compile

    cmake . -DCMAKE_RELEASE=TRUE
    make

To cross-compile for windows you'll need MinGW

    yum install mingw32-gcc-c++ mingw32-libzip mingw32-boost mingw32-qt5-qtbase-devel mingw32-libvorbis
    
Also you need to compile libsndfile yourself with mingw32-configure && make
    
Compile

    mingw32-cmake . -DCMAKE_RELEASE=TRUE
    make

## Usage

    lsd2dsl --lsd /path/to/dictionary.lsd --out /output/directory

There's also a python reimplementation of this: https://github.com/sv99/lsdreader.
