
QT       += core
QT       -= gui
QT       += xml

TARGET = DeviceMng
TEMPLATE = lib

DEFINES += DEBUGLOG COMMON

TOPDIR = $$PWD/../../public/
include($$TOPDIR/app.pri)


MOC_DIR = $$OUT_PWD/moc_tmp
OBJECTS_DIR = $$OUT_PWD/o_tmp
DEFINES += DEVICEMNGLIB_LIBRARY


SOURCES += \
    ShmObject.cpp \
    shm.cpp \
    SemObject.cpp \
    MsgObject.cpp \
    DeviceMng.cpp \
    driver.cpp \
    msg.cpp \
    MsgMng.cpp \
    DeviceMngApi.cpp

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
    devicemnglib_global.h \
    DeviceMngApi.h


#linux-g++ {
#    message("i386")
#    DEFINES += I386
#    INSTALLPATH = ../MainWinDebugEnv/install_pc/lib
#    INSTALLPATH_INCLUDE = ../MainWinDebugEnv/install_pc/include/DeviceMng
#}
#linux-arm-gnueabi-g++ {
#    message("arm")
#    DEFINES += ARM
#    INSTALLPATH = ../MainWinDebugEnv/install_arm/lib
#    INSTALLPATH_INCLUDE = ../MainWinDebugEnv/install_arm/include/DeviceMng
#}

# install
#target.path = $$INSTALL_INCLUDE

#INSTALLS += target
