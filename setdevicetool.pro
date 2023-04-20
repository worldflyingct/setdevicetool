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
    common/hextobin.c \
    ispiocontroller/ispiocontroller.cpp \
    isp485/isp485.cpp \
    ispcollector/ispcollector.cpp \
    iotprogram/isp/ispiotprogram.cpp \
    iotprogram/rj45/rj45iotprogram.cpp \
    stmisp/stmisp.cpp \
    tkmisp/tkmisp.cpp \
    uartassist/uartassist.cpp \
    netassist/netassist.cpp \
    common/common.cpp \
    smartbuilding/22/smartbuilding22.cpp \
    tkg300config/tkg300config.cpp

HEADERS  += mainwindow/mainwindow.h \
    common/yyjson.h \
    common/common.h \
    common/hextobin.h \
    ispiocontroller/ispiocontroller.h \
    isp485/isp485.h \
    ispcollector/ispcollector.h \
    iotprogram/isp/ispiotprogram.h \
    iotprogram/rj45/rj45iotprogram.h \
    stmisp/stmisp.h \
    tkmisp/tkmisp.h \
    uartassist/uartassist.h \
    netassist/netassist.h \
    smartbuilding/22/smartbuilding22.h \
    tkmisp/patch.h \
    tkg300config/tkg300config.h

FORMS    += mainwindow/mainwindow.ui \
    ispiocontroller/ispiocontroller.ui \
    isp485/isp485.ui \
    ispcollector/ispcollector.ui \
    iotprogram/isp/ispiotprogram.ui \
    iotprogram/rj45/rj45iotprogram.ui \
    stmisp/stmisp.ui \
    tkmisp/tkmisp.ui \
    uartassist/uartassist.ui \
    netassist/netassist.ui \
    smartbuilding/22/smartbuilding22.ui \
    tkg300config/tkg300config.ui

DISTFILES +=

RESOURCES += \
    resources/resources.qrc
