
CXX = clang++-mp-3.3
LIBXML2_ROOT ?= /opt/local
GOOGLETEST_ROOT ?= /opt/local

# LDFLAGS += -arch i386

CXXFLAGS += -I$(GOOGLETEST_ROOT)/include
CXXFLAGS += -DGTEST_HAS_TR1_TUPLE=0
CXXFLAGS += -DGTEST_USE_OWN_TR1_TUPLE
CXXFLAGS += -DGTEST_HAS_RTTI=0

CXXFLAGS += -I$(LIBXML2_ROOT)/include/libxml2
CXXFLAGS += -g
CXXFLAGS += -std=c++0x
CXXFLAGS += -fno-rtti
CXXFLAGS += -stdlib=libc++

LDFLAGS += -L$(LIBXML2_ROOT)/lib
LDFLAGS += -L$(GOOGLETEST_ROOT)/lib
LDFLAGS += -liconv
LDFLAGS += -lz
LDFLAGS += -lxml2

GOOGLETEST_LDFLAGS += -lgtest -lgtest_main

play: play.cc element.cpp feed.cpp feed-util.cpp
element.cpp: element.hpp
feed.cpp: feed.hpp

test_element: LDFLAGS+=$(GOOGLETEST_LDFLAGS)
test_element: test_element.cpp element.cpp

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
