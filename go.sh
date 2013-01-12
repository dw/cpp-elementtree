
#g++ -g -o b -L/usr/local/libxml2/lib -lxml2 -I/usr/local/libxml2/include/libxml2 -tsd=c++0x b.cc
#exit


# -fsanitize=address-full,bounds \
/opt/local/bin/clang++-mp-3.3 \
    -liconv \
    -lz \
    -g \
    -std=c++0x \
    -ob \
    -lxml2 \
    -stdlib=libc++ \
    -L/usr/local/libxml2/lib \
    -I/usr/local/libxml2/include/libxml2 \
    \
    b.cc element.cpp uname.cpp && ./b
