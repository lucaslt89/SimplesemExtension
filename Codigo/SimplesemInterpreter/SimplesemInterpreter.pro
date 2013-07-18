#-------------------------------------------------
#
# Project created by QtCreator 2012-06-25T20:11:44
#
#-------------------------------------------------

QT       += core gui
QT       += script
QT += widgets

TARGET = SimplesemInterpreter
TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    ejecutor.cpp \
    hilo.cpp \
    ejemplos.cpp \
    instructionparser.cpp \
    filemanager.cpp

HEADERS  += \
    mainwindow.h \
    ejecutor.h \
    hilo.h \
    ejemplos.h \
    instructionparser.h \
    filemanager.h

FORMS    += \
    mainwindow.ui

OTHER_FILES += \
    SimpleSemSource.txt \
    TODOList.txt \
    parserFer.pl
