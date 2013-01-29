
CXX = clang++-mp-3.3
LIBXML2_ROOT ?= /opt/local
GOOGLETEST_ROOT ?= /opt/local
XAPIAN_ROOT ?= /usr/local/xapian

# LDFLAGS += -arch i386
ifdef RELEASE
CXXFLAGS += -O2
CXXFLAGS += -DNDEBUG
else
CXXFLAGS += -g
endif

CXXFLAGS += -I$(XAPIAN_ROOT)/include
CXXFLAGS += -I$(LIBXML2_ROOT)/include/libxml2
CXXFLAGS += -std=c++0x
CXXFLAGS += -fno-rtti
CXXFLAGS += -stdlib=libc++

LDFLAGS += -L$(XAPIAN_ROOT)/lib
LDFLAGS += -L$(LIBXML2_ROOT)/lib
LDFLAGS += -L$(GOOGLETEST_ROOT)/lib
#LDFLAGS += -liconv
LDFLAGS += -lz
LDFLAGS += -lxml2

GOOGLETEST_LDFLAGS += -lgtest -lgtest_main
GOOGLETEST_CXXFLAGS += -I$(GOOGLETEST_ROOT)/include
GOOGLETEST_CXXFLAGS += -DGTEST_HAS_TR1_TUPLE=0
GOOGLETEST_CXXFLAGS += -DGTEST_USE_OWN_TR1_TUPLE
GOOGLETEST_CXXFLAGS += -DGTEST_HAS_RTTI=0

XAPIAN_LDFLAGS += -L$(XAPIAN_ROOT)/lib -lxapian
XAPIAN_CXXFLAGS += -I$(XAPIAN_ROOT)/include

play: play.cc fileutil.cpp element.cpp feed.cpp feed-util.cpp

indexer: LDFLAGS += $(XAPIAN_LDFLAGS)
indexer: CXXFLAGS += $(XAPIAN_CXXFLAGS)
indexer: indexer.cc fileutil.cpp element.cpp feed.cpp feed-util.cpp

element.cpp: element.hpp
feed.cpp: feed.hpp

test_element: LDFLAGS+=$(GOOGLETEST_LDFLAGS)
test_element: CXXFLAGS+=$(GOOGLETEST_CXXFLAGS)
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
