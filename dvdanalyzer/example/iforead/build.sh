#! /bin/sh

gcc -I. -I../../include -L../../lib \
	../../src/ifodata.c \
	../../src/list.c \
	../../src/list_node.c \
	../../src/pgcanalyzer.c \
	../../src/list_iterator.c \
	../../src/logger.c \
	digiarty_dvd_info.c  \
	--shared -o iforead.dll -ldvdread -ldvdcss \
	-static-libgcc -static-libstdc++
	
	
strip ./iforead.dll
