#include <cassert>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <utility>
#include <vector>

#include "myunit.hpp"

#include "element.hpp"
#include "test_consts.hpp"


using etree::Element;


//
// etree::fromstring()
//


MU_TEST(fromstringEmpty)
{
    auto e = myunit::raises<etree::xml_error>([&]() {
        etree::fromstring("");
    });
    assert(std::string("Start tag expected, '<' not found") == e.what());
}


MU_TEST(fromstring)
{
    auto e = etree::fromstring("<root/>");
    assert(e.tag() == "root");
}


MU_TEST(fromstringParseError)
{
    auto e = myunit::raises<etree::xml_error>([&]() {
        etree::fromstring("corrupt");
    });
    auto expect = "Start tag expected, '<' not found";
    assert(e.what() == std::string(expect));
}


//
// etree::parse()
//


MU_TEST(parseIstream)
{
    std::ifstream ifs;
    ifs.open("testdata/metafilter.rss.xml");
    assert(ifs.is_open());
    auto e = etree::parse(ifs);
    ifs.close();
    assert(e.getroot().tag() == "rss");
}


MU_TEST(parseIstreamCorrupt)
{
    std::ifstream ifs;
    ifs.open("testdata/corrupt.xml");
    assert(ifs.is_open());
    myunit::raises<etree::xml_error>([&]() {
        etree::parse(ifs);
    });
    ifs.close();
}


MU_TEST(parsePath)
{
    auto e = etree::parse("testdata/metafilter.rss.xml");
    assert(e.getroot().tag() == "rss");
}


MU_TEST(parsePathCorrupt)
{
    myunit::raises<etree::xml_error>([&]() {
        etree::parse("testdata/corrupt.xml");
    });
}


MU_TEST(parseFd)
{
    int fd = ::open("testdata/metafilter.rss.xml", O_RDONLY);
    assert(fd != -1);
    auto e = etree::parse(fd);
    ::close(fd);
    assert(e.getroot().tag() == "rss");
}


MU_TEST(parseFdCorrupt)
{
    int fd = ::open("testdata/corrupt.xml", O_RDONLY);
    assert(fd != -1);
    myunit::raises<etree::xml_error>([&]() {
        etree::parse(fd);
    });
    ::close(fd);
}


//
// etree::html::fromstring()
//


MU_TEST(htmlFromstringEmpty)
{
    auto e = myunit::raises<etree::xml_error>([&]() {
        etree::html::fromstring("");
    });
    assert(std::string("Document is empty") == e.what());
}


MU_TEST(htmlFromstring)
{
    auto e = etree::html::fromstring("<p>Hello</p>");
    assert(e.findall(".//p").size() == 1);
}
