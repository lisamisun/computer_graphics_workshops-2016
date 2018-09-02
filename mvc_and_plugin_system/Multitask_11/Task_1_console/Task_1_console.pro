QT += core
QT -= gui

TARGET = Task_1_console
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++11

TEMPLATE = app

SOURCES += main.cpp \
    src/console_observer.cpp \
    ../Task_1/align_project/externals/EasyBMP/src/EasyBMP.cpp \
    ../Task_1/align_project/src/align.cpp \
    ../Task_1/align_project/src/io.cpp \

HEADERS += \
    include/console_observer.h \
    ../Task_1/align_project/externals/EasyBMP/include/EasyBMP.h \
    ../Task_1/align_project/externals/EasyBMP/include/EasyBMP_BMP.h \
    ../Task_1/align_project/externals/EasyBMP/include/EasyBMP_DataStructures.h \
    ../Task_1/align_project/externals/EasyBMP/include/EasyBMP_VariousBMPutilities.h \
    ../Task_1/align_project/include/align.h \
    ../Task_1/align_project/include/io.h \
    ../Task_1/align_project/include/matrix.h \
    ../Task_1/align_project/include/matrix.hpp \
    ../Task_1/align_project/include/observer.h

