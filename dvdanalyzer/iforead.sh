#! /bin/sh

gcc ./example/iforead/main.c ./example/iforead/digiarty_dvd_info.c ./src/ifodata.c  \
./src/list.c ./src/list_iterator.c ./src/list_node.c ./src/logger.c ./src/pgcanalyzer.c \
./src/titleinfo.c -o iforead -I./example/iforead -I./include -L./lib -ldvdcss -ldvdread \
-framework IOKit -framework CoreFoundation \
-arch i386 -Wl,-no_pie  -mmacosx-version-min=10.5 



install_name_tool -change "/Users/digiarty/dvd/dvdanalyzer/lib/libdvdcss.2.dylib" "@executable_path/libdvdcss.2.dylib"  lib/libdvdread.dylib


install_name_tool -change "/Users/digiarty/dvd/dvdanalyzer/lib/libdvdcss.2.dylib" "@executable_path/libdvdcss.2.dylib" iforead 
install_name_tool -change "/Users/digiarty/dvd/dvdanalyzer/lib/libdvdread.4.dylib" "@executable_path/libdvdread.4.dylib" iforead 
