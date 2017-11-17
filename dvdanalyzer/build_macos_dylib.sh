#! /bin/sh

rm -f lib/libdvdcss.dylib
rm -f lib/libdvdcss.2.dylib

#titleinfo.c

gcc \
 -I./include -I./example/iforead -L./lib -ldvdcss -ldvdread \
./example/iforead/digiarty_dvd_info.c \
./src/ifodata.c  ./src/leak_detector_c.c  ./src/list.c ./src/list_iterator.c \
./src/list_node.c ./src/logger.c ./src/pgcanalyzer.c \
\
-Wl,-dynamic,-search_paths_first -Qunused-arguments  \
-dynamiclib -current_version 1.0 -o libiforead.dylib \
-install_name @executable_path/../fws/libiforead.dylib \
-arch i386 -Wl,-no_pie  -mmacosx-version-min=10.5  -v \
-framework CoreFoundation -framework IOKit


