#-------------------------------------------------
#
# Project created by QtCreator 2013-09-04T11:34:21
#
#-------------------------------------------------

TEMPLATE = app

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_SCRIPT = 1
VEIN_DEP_HASH = 1
VEIN_DEP_PROTOBUF = 1
VEIN_DEP_TCP = 1
VEIN_DEP_NET = 1
VEIN_DEP_HELPER = 1

contains(DEFINES, OE_BUILD) {
  message(Openembedded build)
DEFINES += SESSION_PATH=\\\"://target/\\\"
DEFINES += MODMAN_MODULE_PATH=\\\"/usr/lib/zera-modules\\\"

MODMAN_CONFIG_FILES = target/default-session.json \
target/ref-session.json

config_files.files = $$MODMAN_CONFIG_FILES
config_files.path = /etc/zera/modules/sessions
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

  exists($$VEIN_BASEDIR/vein-framework.pri) {
    include($$VEIN_BASEDIR/vein-framework.pri)
  }

}

QT       += core network qml

QT       -= gui

TARGET = zera-modulemanager
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += src/
SOURCES += src/main.cpp \
    src/modulemanager.cpp \
    src/jsonsessionloader.cpp \
    src/modulemanagercontroller.cpp

QMAKE_CXXFLAGS += -Wall -Wshadow

INCLUDEPATH += $$PROTONET_INCLUDEDIR
INCLUDEPATH += $$VIRTUALMODULE_INCLUDEDIR
INCLUDEPATH += $$PROXY_INCLUDEDIR

LIBS += $$PROTONET_LIBDIR
LIBS += $$VIRTUALMODULE_LIBDIR
LIBS += $$PROXY_LIBDIR
LIBS += $$RESOURCE_PROTOBUF_LIBDIR
LIBS += -lzera-proxy -lproto-net-qt -lzera-resourcemanager-protobuf -lMeasurementModuleInterface

HEADERS += src/modulemanager.h \
    src/jsonsessionloader.h \
    src/modulemanagercontroller.h

target.path = /usr/bin
INSTALLS += target

RESOURCES += \
    data1.qrc
