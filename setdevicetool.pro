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
    common/common.cpp \
    common/yyjson.c \
    common/hextobin.c \
    stmisp/stmisp.cpp \
    uartassist/uartassist.cpp \
    netassist/netassist.cpp \
    rs485serialserver/rs485serialserver.cpp

HEADERS  += mainwindow/mainwindow.h \
    common/yyjson.h \
    common/common.h \
    common/hextobin.h \
    stmisp/stmisp.h \
    uartassist/uartassist.h \
    netassist/netassist.h \
    rs485serialserver/rs485serialserver.h

FORMS    += mainwindow/mainwindow.ui \
    stmisp/stmisp.ui \
    uartassist/uartassist.ui \
    netassist/netassist.ui \
    rs485serialserver/rs485serialserver.ui

DISTFILES +=

RESOURCES += \
    resources/resources.qrc
