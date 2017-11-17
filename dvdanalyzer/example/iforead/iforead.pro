TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    digiarty_dvd_info.c \
    ../../src/ifodata.c \
    ../../src/leak_detector_c.c \
    ../../src/list.c \
    ../../src/list_iterator.c \
    ../../src/list_node.c \
    ../../src/logger.c \
    ../../src/pgcanalyzer.c

HEADERS += \
    digiarty_dvd_info.h \
    ../../src/ifodata.h \
    ../../src/leak_detector_c.h \
    ../../src/list.h \
    ../../src/logger.h \
    ../../src/pgcanalyzer.h \
    ../../src/threads.h \
    ../../src/titleinfo.h

win32: LIBS += -L$$PWD/../../lib/ -ldvdread

INCLUDEPATH += $$PWD/../../include
DEPENDPATH += $$PWD/../../include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../lib/dvdread.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../lib/libdvdread.a


win32: LIBS += -L$$PWD/../../lib/ -ldvdcss

INCLUDEPATH += $$PWD/../../include
DEPENDPATH += $$PWD/../../include

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../lib/dvdcss.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../lib/libdvdcss.a

