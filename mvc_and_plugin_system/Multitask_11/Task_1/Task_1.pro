#-------------------------------------------------
#
# Project created by QtCreator 2016-10-08T11:36:41
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Task_1
TEMPLATE = app


SOURCES +=\
        mainwindow.cpp \
    align_project/externals/EasyBMP/src/EasyBMP.cpp \
    align_project/src/align.cpp \
    align_project/src/io.cpp \
    align_project/src/main.cpp

HEADERS  += mainwindow.h \
    align_project/externals/EasyBMP/include/EasyBMP.h \
    align_project/externals/EasyBMP/include/EasyBMP_BMP.h \
    align_project/externals/EasyBMP/include/EasyBMP_DataStructures.h \
    align_project/externals/EasyBMP/include/EasyBMP_VariousBMPutilities.h \
    align_project/include/align.h \
    align_project/include/io.h \
    align_project/include/matrix.h \
    align_project/include/matrix.hpp \
    align_project/include/observer.h \
    API/include/api.h

FORMS    += mainwindow.ui
