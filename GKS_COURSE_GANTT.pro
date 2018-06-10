#-------------------------------------------------
#
# Project created by QtCreator 2018-06-06T09:21:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GKS_COURSE_GANTT
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    logentry.cpp \
    item.cpp \
    datastorage.cpp \
    basefactory.cpp \
    atm.cpp \
    fpm.cpp \
    errorshandler.cpp \
    as.cpp \
    suprule.cpp \
    casechart.cpp

HEADERS += \
        mainwindow.h \
    logentry.h \
    item.h \
    datastorage.h \
    basefactory.h \
    atm.h \
    fpm.h \
    errorshandler.h \
    as.h \
    suprule.h \
    casechart.h

include(xlsx/qtxlsx.pri)
