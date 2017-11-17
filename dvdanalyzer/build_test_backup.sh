#! /bin/sh

gcc \
 -I./include -I./example/iforead -L./lib -ldvdcss -ldvdread \
./example/iforead/main.c ./example/iforead/digiarty_dvd_info.c \
./src/ifodata.c  ./src/leak_detector_c.c  ./src/list.c ./src/list_iterator.c \
./src/list_node.c ./src/logger.c ./src/pgcanalyzer.c ./src/titleinfo.c -o test \
-framework IOKit -framework CoreFoundation \
-arch i386 -Wl,-no_pie  -mmacosx-version-min=10.5  -v


