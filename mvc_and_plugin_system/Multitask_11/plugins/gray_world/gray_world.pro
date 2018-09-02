#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

CONFIG += c++11
TARGET = gray_world
TEMPLATE = lib

DEFINES += GRAY_WORLD_LIBRARY

SOURCES += gray_world.cpp

HEADERS += gray_world.h\
        gray_world_global.h \
        ../../Task_1/API/include/api.h \
        ../../Task_1/align_project/include/matrix.h \
        ../../Task_1/align_project/include/io.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
