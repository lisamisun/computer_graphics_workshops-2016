#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

CONFIG += c++11
TARGET = unsharp
TEMPLATE = lib

DEFINES += UNSHARP_LIBRARY

SOURCES += unsharp.cpp

HEADERS += unsharp.h\
        unsharp_global.h \
        ../../Task_1/API/include/api.h \
        ../../Task_1/align_project/include/matrix.h \
        ../../Task_1/align_project/include/io.h


unix {
    target.path = /usr/lib
    INSTALLS += target
}
