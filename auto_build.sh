#! /bin/sh

################################################################################
# 获取运行环境
################################################################################

# 获取当前目录的绝对路径
project_path=$(cd `dirname $0`; pwd)


################################################################################
# 建立输出目录
################################################################################
prefix_path="${project_path}/local"
if [ ! -d ${prefix_path} ]; then 
    mkdir ${prefix_path}
fi

################################################################################
# 设置环境变量
################################################################################

# 设置pkgconfig搜索路径
export PKG_CONFIG_PATH="${prefix_path}/lib/pkgconfig"


################################################################################
# 通用函数
################################################################################
function quit_config 
{
    cd ${project_path}
    if [ ${1} > 0 ]; then
        echo "exit : ${1}"
    fi
    exit ${1}
}

function check_and_cd
{
    temp="${project_path}/dvdanalyzer/${1}"

    if [ ! -d ${temp} ]; then
        echo "Can't find ${temp}"
        quit_config $LINENO
    else 
        cd ${temp}
    fi
}

function check_return
{
    if [ $? -ne 0 ]; then
        quit_config ${1}
    fi
}


################################################################################
# libdvdcss
################################################################################

if [ ! -f "${prefix_path}/lib/libdvdcss.a" ]; then
    check_and_cd "libdvdcss"

    autoreconf -i
    check_return $LINENO

    ./configure  --prefix=${prefix_path} --enable-static --disable-shared LDFLAGS="-Wno-undef"
    check_return $LINENO

    make clean && make -j8 && make install
    check_return $LINENO
 
    cd ${project_path}
fi


################################################################################
# libdvdread
################################################################################

if [ ! -f "${prefix_path}/lib/libdvdread.a" ]; then
    check_and_cd "libdvdread"

    autoreconf -i
    check_return $LINENO

    ./configure \
        --enable-static --disable-shared \
        --prefix=${prefix_path} \
        LIBS="-Wl,--output-def,libdvdread.def -ldvdcss" \
        LDFLAGS="-Wno-undef -L${prefix_path}/lib" CPPFLAGS=-I${prefix_path}/include \
        --with-libdvdcss

    make clean && make -j8 && make install
    check_return $LINENO

    cd ${project_path}
fi


################################################################################
# libdvdnav
################################################################################

if [ ! -f "${prefix_path}/lib/libdvdnav.a" ]; then
    check_and_cd "libdvdnav"

    autoreconf -i
    check_return $LINENO

    ./configure \
        --enable-static --disable-shared \
        LIBS="-Wl,--output-def,libdvdnav.def -ldvdread -ldvdcss" \
        LDFLAGS="-Wno-undef -L${prefix_path}/lib" CPPFLAGS=-I${prefix_path}/include \
        --prefix=${prefix_path} 

    make clean && make -j8 && make install
    check_return $LINENO

    cd ${project_path}
fi


