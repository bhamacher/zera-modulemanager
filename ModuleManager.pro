#-------------------------------------------------
#
# Project created by QtCreator 2013-09-04T11:34:21
#
#-------------------------------------------------

TEMPLATE = app

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_HASH = 1
VEIN_DEP_PROTOBUF = 1
VEIN_DEP_TCP = 1
VEIN_DEP_NET = 1
VEIN_DEP_HELPER = 1
VEIN_DEP_SCRIPT = 1
VEIN_DEP_QML = 1
VEIN_DEP_LOGGER = 1

VEIN_BASEDIR = /work/qt_projects/vein-framework

contains(DEFINES, OE_BUILD) {
  message(Openembedded build)
DEFINES += SESSION_PATH=\\\"/etc/zera/modules/sessions/\\\"
DEFINES += MODMAN_MODULE_PATH=\\\"/usr/lib/zera-modules/\\\"
DEFINES += MODMAN_CUSTOMERDATA_PATH=\\\"/home/operator/customerdata/\\\"

MODMAN_CONFIG_FILES = target/com5003-meas-session.json \
target/com5003-ref-session.json \
target/com5003-ced-session.json

config_files.files = $$MODMAN_CONFIG_FILES
config_files.path = /etc/zera/modules/sessions
INSTALLS += config_files

  exists(../oe-build.pri) {
    include(../oe-build.pri)
  }
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
    src/customerdatasystem.cpp

QMAKE_CXXFLAGS += -Wall -Wshadow

INCLUDEPATH += $$VIRTUALMODULE_INCLUDEDIR
INCLUDEPATH += $$PROXY_INCLUDEDIR
INCLUDEPATH += $$VEIN_QML_INCLUDEDIR

LIBS += $$VIRTUALMODULE_LIBDIR
LIBS += $$PROXY_LIBDIR
LIBS += $$RESOURCE_PROTOBUF_LIBDIR
LIBS += $$SCPI_LIBDIR
LIBS += -lzera-proxy -lzera-resourcemanager-protobuf -lMeasurementModuleInterface

HEADERS += src/modulemanager.h \
    src/jsonsessionloader.h \
    src/modulemanagercontroller.h \
    src/moduleeventhandler.h \
    src/customerdatasystem.h

target.path = /usr/bin
INSTALLS += target

RESOURCES += \
    data1.qrc
