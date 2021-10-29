#-------------------------------------------------
#
# Project created by QtCreator 2021-10-30T00:32:19
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = setdevicetool
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    cjson.c \
    ispiocontroller.cpp \
    common.c \
    isp485.cpp

HEADERS  += mainwindow.h \
    cjson.h \
    ispiocontroller.h \
    common.h \
    isp485.h

FORMS    += mainwindow.ui \
    ispiocontroller.ui \
    isp485.ui
