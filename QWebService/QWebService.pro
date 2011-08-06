#-------------------------------------------------
#
# Project created by QtCreator 2011-05-11T11:13:39
# Tomasz 'sierdzio' Siekierda
# sierdzio@gmail.com
#-------------------------------------------------

QT       += network xml

QT       -= gui

TARGET = QWebService

TEMPLATE = lib

CONFIG += dll

DEFINES += QWEBSERVICE_LIBRARY

VERSION = 0.2.2

DESTDIR = ../lib

SOURCES += sources/qwebserviceabstract.cpp \
    sources/qwebservicereaderabstract.cpp \
    sources/qwebservicemethodabstract.cpp \
    sources/qwsdl.cpp \
    sources/qsoapmessage.cpp

HEADERS += headers/QWebService_global.h \
    headers/qwebserviceabstract.h \
    headers/qwebservicereaderabstract.h \
    headers/qwebservicemethodabstract.h \
    headers/qwsdl.h \
    headers/qsoapmessage.h \
    headers/QWebService

symbian {
    #Symbian specific definitions
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE45E3457
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = QWebService.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}