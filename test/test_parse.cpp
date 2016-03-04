
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"

#include "test_consts.hpp"


using etree::Element;


//
// etree::fromstring()
//


TEST_CASE("fromstringEmpty", "[parse]")
{
    REQUIRE_THROWS_AS(etree::fromstring(""), etree::xml_error);
    //REQUIRE(std::string("Start tag expected, '<' not found") == e.what());
}


TEST_CASE("fromstring", "[parse]")
{
    auto e = etree::fromstring("<root/>");
    REQUIRE(e.tag() == "root");
}


TEST_CASE("fromstringParseError", "[parse]")
{
    REQUIRE_THROWS_AS(etree::fromstring("corrupt"), etree::xml_error);
    //auto expect = "Start tag expected, '<' not found";
    //REQUIRE(e.what() == std::string(expect));
}


//
// etree::parse()
//


TEST_CASE("parseIstream", "[parse]")
{
    std::ifstream ifs;
    ifs.open("testdata/metafilter.rss.xml");
    REQUIRE(ifs.is_open());
    auto e = etree::parse(ifs);
    ifs.close();
    REQUIRE(e.getroot().tag() == "rss");
}


TEST_CASE("parseIstreamCorrupt", "[parse]")
{
    std::ifstream ifs;
    ifs.open("testdata/corrupt.xml");
    REQUIRE(ifs.is_open());
    REQUIRE_THROWS_AS(etree::parse(ifs), etree::xml_error);
    ifs.close();
}


TEST_CASE("parsePath", "[parse]")
{
    auto e = etree::parse("testdata/metafilter.rss.xml");
    REQUIRE(e.getroot().tag() == "rss");
}


TEST_CASE("parsePathCorrupt", "[parse]")
{
    REQUIRE_THROWS_AS(etree::parse("testdata/corrupt.xml"), etree::xml_error);
}


TEST_CASE("parseFd", "[parse]")
{
    int fd = ::open("testdata/metafilter.rss.xml", O_RDONLY);
    REQUIRE(fd != -1);
    auto e = etree::parse(fd);
    ::close(fd);
    REQUIRE(e.getroot().tag() == "rss");
}


TEST_CASE("parseFdCorrupt", "[parse]")
{
    int fd = ::open("testdata/corrupt.xml", O_RDONLY);
    REQUIRE(fd != -1);
    REQUIRE_THROWS_AS(etree::parse(fd), etree::xml_error);
    ::close(fd);
}


//
// etree::html::fromstring()
//


TEST_CASE("htmlFromstringEmpty", "[parse]")
{
    REQUIRE_THROWS_AS(etree::html::fromstring(""), etree::xml_error);
    //REQUIRE(std::string("Document is empty") == e.what());
}


TEST_CASE("htmlFromstring", "[parse]")
{
    auto e = etree::html::fromstring("<p>Hello</p>");
    REQUIRE(e.findall(".//p").size() == 1);
}
