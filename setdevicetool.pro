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
        mainwindow/mainwindow.cpp \
    common/cjson.c \
    ispiocontroller/ispiocontroller.cpp \
    common/common.c \
    isp485/isp485.cpp

HEADERS  += mainwindow/mainwindow.h \
    common/cjson.h \
    ispiocontroller/ispiocontroller.h \
    common/common.h \
    isp485/isp485.h

FORMS    += mainwindow/mainwindow.ui \
    ispiocontroller/ispiocontroller.ui \
    isp485/isp485.ui

DISTFILES += \
    .gitignore \
    LICENSE \
    README.md

RESOURCES += \
    resources/resources.qrc