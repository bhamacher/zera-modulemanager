#-------------------------------------------------
#
# Project created by QtCreator 2013-09-04T11:34:21
#
#-------------------------------------------------

VERSION = 1.1.0

TEMPLATE = app

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_HASH = 1
VEIN_DEP_TCP = 1 #required by libzeraproxy
VEIN_DEP_TCP2 = 1
VEIN_DEP_NET2 = 1
VEIN_DEP_HELPER = 1
VEIN_DEP_SCRIPT = 1
VEIN_DEP_QML = 1
VEIN_DEP_LOGGER = 1
VEIN_DEP_CRYPTOBRIDGE = 1

contains(DEFINES, OE_BUILD) {
  message(Openembedded build)
DEFINES += MODMAN_SESSION_PATH=\\\"/etc/zera/modules/sessions/\\\"
DEFINES += MODMAN_MODULE_PATH=\\\"/usr/lib/zera-modules/\\\"
DEFINES += MODMAN_CUSTOMERDATA_PATH=\\\"/home/operator/customerdata/\\\"
DEFINES += MODMAN_CONFIG_FILE=\\\"/etc/zera/modules/modulemanager_config.json\\\"

MODMAN_SESSION_FILES = target/com5003-meas-session.json \
target/com5003-ref-session.json \
target/com5003-ced-session.json \
target/mt310s2-meas-session.json

session_files.files = $$MODMAN_SESSION_FILES
session_files.path = /etc/zera/modules/sessions

config_files.files = modulemanager_config.json
config_files.path = /etc/zera/modules

INSTALLS += session_files config_files

  exists(../oe-build.pri) {
    include(../oe-build.pri)
  }
}
else {
DEFINES += MODMAN_SESSION_PATH=\\\"$$PWD/\\\"
# DEFINE your own MODMAN_MODULE_PATH in your .pri file

  exists(modulemanager.user.pri) {
    include(modulemanager.user.pri)
  } else {
    message("No user build configuration found, continuing")
  }

  exists($$VEIN_BASEDIR/vein-framework.pri) {
    include($$VEIN_BASEDIR/vein-framework.pri)
  } else {
    error("Dependency vein-framework.pri not found")
  }
}

QT       += core network qml concurrent
QT       -= gui

TARGET = zera-modulemanager
CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += src/
SOURCES += src/main.cpp \
    src/modulemanager.cpp \
    src/jsonsessionloader.cpp \
    src/modulemanagercontroller.cpp \
    src/moduleeventhandler.cpp \
    src/customerdatasystem.cpp \
    src/priorityarbitrationsystem.cpp \
    src/zeradblogger.cpp \
    src/licensesystem.cpp \
    src/diriteratorworker.cpp

QMAKE_CXXFLAGS += -Wall -Wshadow

INCLUDEPATH += $$VIRTUALMODULE_INCLUDEDIR
INCLUDEPATH += $$PROXY_INCLUDEDIR
INCLUDEPATH += $$VEIN_QML_INCLUDEDIR

LIBS += $$ZERA_CLASSES_LIBDIR
LIBS += $$RESOURCE_PROTOBUF_LIBDIR
LIBS += $$SCPI_LIBDIR
LIBS += -lzera-proxy -lzera-resourcemanager-protobuf -lMeasurementModuleInterface

HEADERS += src/modulemanager.h \
    src/jsonsessionloader.h \
    src/modulemanagercontroller.h \
    src/moduleeventhandler.h \
    src/customerdatasystem.h \
    src/modman_util.h \
    src/priorityarbitrationsystem.h \
    src/zeradblogger.h \
    src/licensesystem.h \
    src/diriteratorworker.h

target.path = /usr/bin
INSTALLS += target

RESOURCES += \
    data1.qrc



