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
    Combiner.cpp \
    XmlSaveLoad.cpp \
    progressbardialog.cpp \
    EncryptionAlgorithm/Gost.cpp \

HEADERS  += mainwindow.h \
    Combiner.h \
    XmlSaveLoad.h \
    progressbardialog.h \
    FileObject.h \
    EncryptionAlgorithm/Gost.h \
    z_qbytearray.h

FORMS    += mainwindow.ui \
    progressbardialog.ui

CONFIG   += c++11

QMAKE_CXXFLAGS += -g \
                  -gstabs+

