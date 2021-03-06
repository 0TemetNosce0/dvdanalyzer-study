######################################################################
# Automatically generated by qmake (3.1) Fri Nov 17 11:05:34 2017
######################################################################

TEMPLATE = app
TARGET = libdvdread
INCLUDEPATH += .

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += msvc/config.h \
           src/bswap.h \
           src/dvd_input.h \
           src/dvdread_internal.h \
           src/md5.h \
           msvc/include/dlfcn.h \
           msvc/include/dvdnav_internal.h \
           msvc/include/getopt.h \
           msvc/include/inttypes.h \
           msvc/include/os_types.h \
           msvc/include/timer.h \
           msvc/include/unistd.h \
           src/dvdread/bitreader.h \
           src/dvdread/dvd_iso9660.h \
           src/dvdread/dvd_reader.h \
           src/dvdread/dvd_udf.h \
           src/dvdread/ifo_print.h \
           src/dvdread/ifo_read.h \
           src/dvdread/ifo_types.h \
           src/dvdread/nav_print.h \
           src/dvdread/nav_read.h \
           src/dvdread/nav_types.h \
           msvc/contrib/dirent/dirent.h \
           msvc/contrib/timer/timer.h \
           msvc/include/pthreads/pthread.h \
           msvc/include/pthreads/sched.h \
           msvc/include/sys/time.h \
           /msvc/config.h \
           /msvc/include/inttypes.h \
           /msvc/include/unistd.h \
           /msvc/include/dlfcn.h \
           msvc/contrib/dlfcn.c \
           /msvc/include/sys/time.h \
           /msvc/contrib/dirent/dirent.h \
           /msvc/include/getopt.h \
           /msvc/include/pthreads/pthread.h \
           /msvc/include/pthreads/sched.h
SOURCES += src/bitreader.c \
           src/dvd_input.c \
           src/dvd_iso9660.c \
           src/dvd_reader.c \
           src/dvd_udf.c \
           src/ifo_print.c \
           src/ifo_read.c \
           src/md5.c \
           src/nav_print.c \
           src/nav_read.c \
           msvc/contrib/bcopy.c \
           msvc/contrib/dlfcn.c \
           msvc/contrib/getopt.c \
           msvc/contrib/dirent/dirent.c \
           msvc/contrib/timer/timer.c
