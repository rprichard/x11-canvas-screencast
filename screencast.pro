QT       += core gui

TARGET = screencast
CONFIG   += console
CONFIG   -= app_bundle

CONFIG += link_pkgconfig
PKGCONFIG += x11 xfixes

TEMPLATE = app

SOURCES += \
    main.cpp \
    MurmurHash3.cpp \
    Cursor.cpp \
    CaptureConfig.cpp \
    X11Bridge.cpp

HEADERS += \
    MurmurHash3.h \
    Cursor.h \
    CaptureConfig.h \
    X11Bridge.h

# HACK: Avoid stripping the scripts.
QMAKE_STRIP =

target.path = /
scripts.path = /
scripts.files += \
    player.js \
    pack_animation.py

INSTALLS += \
    scripts \
    target
