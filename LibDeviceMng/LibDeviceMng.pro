
QT       += core
QT       -= gui
QT       += xml

TARGET = DeviceMng
TEMPLATE = lib

DEFINES += DEBUGLOG COMMON ZCOMMONLIB PTCOMMLIB

TOPDIR = $$PWD/../../public/
include($$TOPDIR/app.pri)


MOC_DIR = $$OUT_PWD/moc_tmp
OBJECTS_DIR = $$OUT_PWD/o_tmp
DEFINES += DEVICEMNGLIB_LIBRARY


SOURCES += \
    DeviceMngApi.cpp

HEADERS += \
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
