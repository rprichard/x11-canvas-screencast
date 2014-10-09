QT       += core gui

TARGET = screencast
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
    CaptureConfig.cpp \
    CursorCommon.cpp \
    MurmurHash3.cpp \
    main.cpp

macx {
    LIBS += -framework AppKit

    OBJECTIVE_SOURCES += \
        CapsLockStateOSX.mm \
        CursorOSX.mm

} else {
    SOURCES += \
        CapsLockStateX11.cpp \
        CursorX11.cpp

    CONFIG += link_pkgconfig
    PKGCONFIG += x11 xfixes
}

HEADERS += \
    CaptureConfig.h \
    CapsLockState.h \
    Cursor.h \
    MurmurHash3.h

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
