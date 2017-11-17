需要安装的附加工具（用MSYS命令'pacman -S *'安装）：
1.autoconf
2.m4
3.automake
4.libtool
5.pkg-config

PS:
aclocal是一个perl脚本程序，它的定义是：aclocal - create aclocal.m4 by scanning configure.ac。
autoconf从configure.in这个列举编译软件时所需要的各种参数的模板文件中创建configure。
autoconf需要GNU m4宏处理器来处理aclocal.m4，以生成configure脚本。

pkg-config的作用：一般来说，如果库的头文件不在 /usr/include 目录中，那么在编译的时候需要用 -I 参数指定其路径。由于同一个库在不同系统上可能位于不同的目录下，用户安装库的时候也可以将库安装在不同的目录下，所以即使使用同一个库，由于库的路径的不同，造成了用 -I 参数指定的头文件的路径也可能不同，其结果就是造成了编译命令界面的不统一。如果使用 -L 参数，也会造成连接界面的不统一。编译和连接界面不统一会为库的使用带来麻烦。
为了解决编译和连接界面不统一的问题，人们找到了一些解决办法。其基本思想就是：事先把库的位置信息等保存起来，需要的时候再通过特定的工具将其中有用的信息提取出来供编译和连接使用。这样，就可以做到编译和连接界面的一致性。其中，目前最为常用的库信息提取工具就是下面介绍的 pkg-config。
pkg-config 是通过库提供的一个 .pc 文件获得库的各种必要信息的，包括版本信息、编译和连接需要的参数等。这些信息可以通过 pkg-config 提供的参数单独提取出来直接供编译器和连接器使用。


mplayer的configure命令：
./configure --prefix=/f/source/pandora/ --enable-debug=3 --extra-cflags=-I/f/source/pandora/local/include --extra-ldflags="-L/f/source/pandora/local/lib -ldvdnav -ldvdread -ldvdcss"
