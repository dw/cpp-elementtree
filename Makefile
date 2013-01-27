
CXX = clang++-mp-3.3
LIBXML2_ROOT ?= /opt/local

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

noexist:

docs: noexist
	rm -rf docs
	doxygen

pushdocs: noexist
	rm -rf docs
	mkdir docs
	cp -a .git docs/.git
	( cd docs; git checkout gh-pages; )
	doxygen
	( cd docs; git add -A .; git ci -am "Update documentation"; )
	( cd docs; git push origin gh-pages; )
	git pull

clean:
	rm -f play *.o *.a
