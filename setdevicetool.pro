#-------------------------------------------------
#
# Project created by QtCreator 2021-10-30T00:32:19
#
#-------------------------------------------------

QT       += core gui
QT       += network
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = setdevicetool
TEMPLATE = app

RC_ICONS = resources/favicon.ico

SOURCES += main.cpp\
    mainwindow/mainwindow.cpp \
    common/yyjson.c \
    common/common.c \
    common/hextobin.c \
    ispiocontroller/ispiocontroller.cpp \
    isp485/isp485.cpp \
    ispcollector/ispcollector.cpp \
    ispiotprogram/ispiotprogram.cpp \
    stmisp/stmisp.cpp \
    uartassist/uartassist.cpp \
    netassist/netassist.cpp

HEADERS  += mainwindow/mainwindow.h \
    common/yyjson.h \
    common/common.h \
    common/hextobin.h \
    ispiocontroller/ispiocontroller.h \
    isp485/isp485.h \
    ispcollector/ispcollector.h \
    ispiotprogram/ispiotprogram.h \
    stmisp/stmisp.h \
    uartassist/uartassist.h \
    netassist/netassist.h

FORMS    += mainwindow/mainwindow.ui \
    ispiocontroller/ispiocontroller.ui \
    isp485/isp485.ui \
    ispcollector/ispcollector.ui \
    ispiotprogram/ispiotprogram.ui \
    stmisp/stmisp.ui \
    uartassist/uartassist.ui \
    netassist/netassist.ui

DISTFILES +=

RESOURCES += \
    resources/resources.qrc
