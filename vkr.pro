#-------------------------------------------------
#
# Project created by QtCreator 2014-11-26T12:33:09
#
#-------------------------------------------------

QT       += core gui\
            concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vkr
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    EncryptionAlgorithm/EncryptionAlgorithm.cpp \
    Combiner.cpp \
    XmlSaveLoad.cpp \
    progressbardialog.cpp

HEADERS  += mainwindow.h \
    EncryptionAlgorithm/EncryptionAlgorithm.h \
    Combiner.h \
    FileSystemObject.h \
    FileStructure.h \
    XmlSaveLoad.h \
    progressbardialog.h

FORMS    += mainwindow.ui \
    progressbardialog.ui

CONFIG   += c++11
