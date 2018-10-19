TARGET = eb600keypad
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/kbddrivers
target.path = $$[QT_INSTALL_PLUGINS]/kbddrivers
INSTALLS += target

HEADERS = eb600keypaddriverplugin.h eb600keypadhandler.h
SOURCES = eb600keypaddriverplugin.cpp eb600keypadhandler.cpp

