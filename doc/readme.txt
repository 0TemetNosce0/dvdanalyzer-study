��Ҫ��װ�ĸ��ӹ��ߣ���MSYS����'pacman -S *'��װ����
1.autoconf
2.m4
3.automake
4.libtool
5.pkg-config

PS:
aclocal��һ��perl�ű��������Ķ����ǣ�aclocal - create aclocal.m4 by scanning configure.ac��
autoconf��configure.in����оٱ������ʱ����Ҫ�ĸ��ֲ�����ģ���ļ��д���configure��
autoconf��ҪGNU m4�괦����������aclocal.m4��������configure�ű���

pkg-config�����ã�һ����˵��������ͷ�ļ����� /usr/include Ŀ¼�У���ô�ڱ����ʱ����Ҫ�� -I ����ָ����·��������ͬһ�����ڲ�ͬϵͳ�Ͽ���λ�ڲ�ͬ��Ŀ¼�£��û���װ���ʱ��Ҳ���Խ��ⰲװ�ڲ�ͬ��Ŀ¼�£����Լ�ʹʹ��ͬһ���⣬���ڿ��·���Ĳ�ͬ��������� -I ����ָ����ͷ�ļ���·��Ҳ���ܲ�ͬ��������������˱����������Ĳ�ͳһ�����ʹ�� -L ������Ҳ��������ӽ���Ĳ�ͳһ����������ӽ��治ͳһ��Ϊ���ʹ�ô����鷳��
Ϊ�˽����������ӽ��治ͳһ�����⣬�����ҵ���һЩ����취�������˼����ǣ����Ȱѿ��λ����Ϣ�ȱ�����������Ҫ��ʱ����ͨ���ض��Ĺ��߽��������õ���Ϣ��ȡ���������������ʹ�á��������Ϳ���������������ӽ����һ���ԡ����У�Ŀǰ��Ϊ���õĿ���Ϣ��ȡ���߾���������ܵ� pkg-config��
pkg-config ��ͨ�����ṩ��һ�� .pc �ļ���ÿ�ĸ��ֱ�Ҫ��Ϣ�ģ������汾��Ϣ�������������Ҫ�Ĳ����ȡ���Щ��Ϣ����ͨ�� pkg-config �ṩ�Ĳ���������ȡ����ֱ�ӹ���������������ʹ�á�


mplayer��configure���
./configure --prefix=/f/source/pandora/ --enable-debug=3 --extra-cflags=-I/f/source/pandora/local/include --extra-ldflags="-L/f/source/pandora/local/lib -ldvdnav -ldvdread -ldvdcss"
