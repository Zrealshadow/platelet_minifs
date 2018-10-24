QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    fs_bitmap.cpp \
    fs_base.cpp \
    fs_core.cpp \
    fs_file.cpp \
    app_console.cpp \
    fs_macro.cpp \
    func_core.cpp \
    sess_core.cpp

HEADERS += \
    fs_bitmap.h \
    fs_macro.h \
    fs_base.h \
    fs_debug.h \
    fs_core.h \
    fs_file.h \
    fs_hash.h \
    app_console.h \
    version.h \
    sess_core.h \
    func_core.h \
    func_macro.h
