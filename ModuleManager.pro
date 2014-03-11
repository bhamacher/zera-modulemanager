#-------------------------------------------------
#
# Project created by QtCreator 2013-09-04T11:34:21
#
#-------------------------------------------------

# see user defined paths
include(../../include/project-paths.pri)

QT       += core network

QT       -= gui

TARGET = zera-modulemanager
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += src/
SOURCES += src/main.cpp \
    src/modulemanager.cpp

QMAKE_CXXFLAGS += -Wall -Wshadow

INCLUDEPATH += $$VEIN_INCLUDEDIR
LIBS += $$VEIN_LIBDIR -lvein-qt
INCLUDEPATH += $$PROTONET_INCLUDEDIR
LIBS += $$PROTONET_LIBDIR -lproto-net-qt
INCLUDEPATH += $$VEIN_PROTOBUF_INLCUDEDIR
LIBS += $$VEIN_PROTOBUF_LIBDIR -lvein-qt-protobuf
INCLUDEPATH += $$VEIN_TCP_INCLUDEDIR
LIBS += $$VEIN_TCP_LIBDIR -lvein-tcp-overlay
INCLUDEPATH += $$VIRTUALMODULE_INCLUDEDIR
LIBS += $$VIRTUALMODULE_LIBDIR -lMeasurementModuleInterface
INCLUDEPATH += $$PROXY_INCLUDEDIR
LIBS+= $$PROXY_LIBDIR -lzera-proxy

LIBS += -lprotobuf

HEADERS += src/modulemanager.h

target.path = /usr/bin
INSTALLS += target
