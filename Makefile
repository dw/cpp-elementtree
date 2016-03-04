
UNAME = $(shell uname)

ifdef RELEASE
CXXFLAGS += -O2
CXXFLAGS += -DNDEBUG
endif

CXXFLAGS += -Wunused -Wall
CXXFLAGS += -g -fno-omit-frame-pointer
CXXFLAGS += -std=c++0x
# CXXFLAGS += -stdlib=libc++
CXXFLAGS += $(shell pkg-config --cflags libxml-2.0)

ifdef LTO
CXXFLAGS += -Os
CXXFLAGS += -flto
LDFLAGS += -flto
endif

LDFLAGS += -lz
LDFLAGS += -lxml2
LDFLAGS += $(shell pkg-config --libs libxml-2.0)

all: targets


TARGETS += test_main
test_main: \
		test_main.cpp \
		test_attrib.o \
		test_element.o \
		test_feed.o \
		test_nullable.o \
		test_parse.o \
		test_qname.o \
		test_xpath.o \
		element.o \
		feed.o \
		feed-util.o

TARGETS += convert_feed
convert_feed: \
	convert_feed.cpp \
	element.o \
	feed.o \
	feed-util.o

TARGETS += sanitize
sanitize: \
	sanitize.cpp \
	element.o \
	feed.o \
	feed-util.o

element.cpp: element.hpp
feed.cpp: feed.hpp

coverage:
	$(MAKE) clean
	CXXFLAGS=--coverage $(MAKE) test_main
	./test_main
	lcov --directory . --base-directory . --gcov-tool ./llvm-gcov.sh --capture -o cov.info
	genhtml cov.info -o output
	open output/index.html

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
	rm -rf $(TARGETS) *.o *.a output *.gcda *.gcno cov.info docs *.dSYM

targets: $(TARGETS)
