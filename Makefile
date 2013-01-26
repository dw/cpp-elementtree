
CXX = clang++-mp-3.3
LIBXML2_ROOT ?= /usr/local/libxml2

CXXFLAGS += -I$(LIBXML2_ROOT)/include/libxml2
CXXFLAGS += -g
CXXFLAGS += -std=c++0x
CXXFLAGS += -fno-rtti
CXXFLAGS += -stdlib=libc++

LDFLAGS += -L$(LIBXML2_ROOT)/lib
LDFLAGS += -liconv
LDFLAGS += -lz
LDFLAGS += -lxml2

play: play.cc element.cpp feed.cpp feed-util.cpp
element.cpp: element.hpp
feed.cpp: feed.hpp

clean:
	rm -f play *.o *.a
