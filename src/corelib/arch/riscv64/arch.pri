#
# RISC-V 64 architecture
#

unix:SOURCES += ../generic/qatomic_generic_unix.cpp
win32:SOURCES += ../generic/qatomic_generic_windows.cpp

QMAKE_CXXFLAGS += -fpermissive
