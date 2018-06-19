#-------------------------------------------------
#
# Project created by QtCreator 2018-06-18T14:54:07
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = nfc-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sqlitehelper.cpp \
    nfchelper.cpp \
    nfc-utils.c

HEADERS  += mainwindow.h \
    sqlitehelper.h \
    nfchelper.h \
    nfc-utils.h

FORMS    += mainwindow.ui
