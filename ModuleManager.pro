#-------------------------------------------------
#
# Project created by QtCreator 2013-09-04T11:34:21
#
#-------------------------------------------------

contains(DEFINES, OE_BUILD) {
  message(Openembedded build)
DEFINES += SESSION_PATH=\\\"/etc/zera/modules/sessions/\\\"
DEFINES += MODMAN_MODULE_PATH=\\\"/usr/lib/zera-modules\\\"

MODMAN_CONFIG_FILES = target/default-session.json \
target/ref-session.json

config_files.files = $$MODMAN_CONFIG_FILES
config_files.path = $$SESSION_PATH
INSTALLS += config_files
}
else {
DEFINES += SESSION_PATH=\\\"$$PWD/\\\"
# DEFINE your own MODMAN_MODULE_PATH in your .pri file

  exists(modulemanager.user.pri) {
    include(modulemanager.user.pri)
  }
  exists(../../include/project-paths.pri) {
    include(../../include/project-paths.pri)
  }
}

QT       += core network

QT       -= gui

TARGET = zera-modulemanager
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += src/
SOURCES += src/main.cpp \
    src/modulemanager.cpp \
    src/jsonsessionloader.cpp

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

HEADERS += src/modulemanager.h \
    src/jsonsessionloader.h

target.path = /usr/bin
INSTALLS += target

RESOURCES += \
    data1.qrc
