
UNAME = $(shell uname)

CXX = c++

ifeq ($(UNAME), Darwin)
CXXFLAGS += -stdlib=libc++
endif

# LDFLAGS += -arch i386
ifdef RELEASE
CXXFLAGS += -O2
CXXFLAGS += -DNDEBUG
else
endif
CXXFLAGS += -g

CXXFLAGS += $(shell pkg-config --cflags libxml-2.0)
CXXFLAGS += -std=c++0x
CXXFLAGS += -fno-rtti
CXXFLAGS += -stdlib=libc++

LDFLAGS += $(shell pkg-config --libs libxml-2.0)
#LDFLAGS += -liconv
LDFLAGS += -lz
LDFLAGS += -lxml2

play: play.cc fileutil.cpp element.cpp feed.cpp feed-util.cpp

element.cpp: element.hpp
feed.cpp: feed.hpp

test_element: test_element.cpp element.cpp

coverage:
	$(MAKE) clean
	CXXFLAGS=--coverage $(MAKE) test_element
	./test_element
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
	rm -rf test_element play *.o *.a output *.gcda *.gcno cov.info docs *.dSYM
