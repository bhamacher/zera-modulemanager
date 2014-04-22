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
LIBS += $$VEIN_LIBDIR
INCLUDEPATH += $$PROTONET_INCLUDEDIR
LIBS += $$PROTONET_LIBDIR
INCLUDEPATH += $$VEIN_PROTOBUF_INLCUDEDIR
LIBS += $$VEIN_PROTOBUF_LIBDIR
INCLUDEPATH += $$VEIN_TCP_INCLUDEDIR
LIBS += $$VEIN_TCP_LIBDIR
INCLUDEPATH += $$VIRTUALMODULE_INCLUDEDIR
LIBS += $$VIRTUALMODULE_LIBDIR
INCLUDEPATH += $$PROXY_INCLUDEDIR
LIBS+= $$PROXY_LIBDIR

LIBS += $$RESOURCE_PROTOBUF_LIBDIR

LIBS += -lzera-proxy -lzera-resourcemanager-protobuf -lvein-qt -lprotobuf -lproto-net-qt -lvein-qt-protobuf -lvein-tcp-overlay -lMeasurementModuleInterface

HEADERS += src/modulemanager.h

target.path = /usr/bin
INSTALLS += target
