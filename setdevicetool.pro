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

RC_ICONS = resources/favicon.ico

SOURCES += main.cpp\
    mainwindow/mainwindow.cpp \
    common/cjson.c \
    ispiocontroller/ispiocontroller.cpp \
    common/common.c \
    isp485/isp485.cpp \
    isplocation/isplocation.cpp \
    ispiotprogram/ispiotprogram.cpp \
    stmisp/stmisp.cpp \
    common/hextobin.c

HEADERS  += mainwindow/mainwindow.h \
    common/cjson.h \
    ispiocontroller/ispiocontroller.h \
    common/common.h \
    isp485/isp485.h \
    isplocation/isplocation.h \
    ispiotprogram/ispiotprogram.h \
    stmisp/stmisp.h \
    common/hextobin.h

FORMS    += mainwindow/mainwindow.ui \
    ispiocontroller/ispiocontroller.ui \
    isp485/isp485.ui \
    isplocation/isplocation.ui \
    ispiotprogram/ispiotprogram.ui \
    stmisp/stmisp.ui

DISTFILES +=

RESOURCES += \
    resources/resources.qrc
