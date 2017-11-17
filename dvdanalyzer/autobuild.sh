# 更新库
git pull

#编译libdvdcss
cd libdvdcss
autoreconf -i
./configure  --prefix=$HOME/dvd/dvdanalyzer CFLAGS=" -arch i386 -Wl,-no_pie  -mmacosx-version-min=10.5 "
make uninstall
make clean
make && make install
cd ..


#编译libdvdread
cd libdvdread
autoreconf -i
./configure --enable-static --disable-shared --prefix=$HOME/dvd/dvdanalyzer --with-libdvdcss LDFLAGS=-L$HOME/dvd/dvdanalyzer/lib CPPFLAGS=-I$HOME/dvd/dvdanalyzer/include LIBS=-ldvdcss CFLAGS=" -arch i386 -Wl,-no_pie  -mmacosx-version-min=10.5 "
make uninstall
make clean
make && make install
cd ..


