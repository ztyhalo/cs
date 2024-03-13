
QT       += core
QT       -= gui
QT       += xml

TARGET = DeviceMng
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

#CONFIG += release
#DEFINES += QT_NO_DEBUG_OUTPUT

DEFINES += DEBUGLOG COMMON

TOPDIR = $$PWD/../../public/
include($$TOPDIR/app.pri)
contains(DEFINES,ARM){
    DESTDIR = $$TOPDIR/../bin/platform
#    target.path = /opt/platform
#    !isEmpty(target.path): INSTALLS = target
}

SOURCES += main.cpp \
    ShmObject.cpp \
    shm.cpp \
    SemObject.cpp \
    MsgObject.cpp \
    DeviceMng.cpp \
    driver.cpp \
    msg.cpp \
    MsgMng.cpp \
    app.cpp


HEADERS += \
    ShmObject.h \
    shm.h \
    define.h \
    SemObject.h \
    MsgObject.h \
    DeviceMng.h \
    driver.h \
    msgtype.h \
    msg.h \
    MsgMng.h \
    app.h

linux-g++ {
    message("i386")
    DEFINES += I386
    APP_INSTALL_PATH_PLATFORM = ../MainWinDebugEnv/install_pc/bin/platform
}
#linux-arm-gnueabi-g++ {
#    message("arm")
#    DEFINES += ARM
#    APP_INSTALL_PATH_PLATFORM = ../MainWinDebugEnv/install_arm/bin/platform
#}
message("arm")
DEFINES += ARM
APP_INSTALL_PATH_PLATFORM = ../MainWinDebugEnv/install_arm/bin/platform
target.path = $$APP_INSTALL_PATH_PLATFORM
INSTALLS += target
