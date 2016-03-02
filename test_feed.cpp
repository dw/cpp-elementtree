/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <utility>
#include <vector>

#include "myunit.hpp"
#include "element.hpp"
#include "feed.hpp"
#include "test_consts.hpp"
#include "feed.hpp"

using std::pair;
using std::vector;
using std::string;

etree::Element atomFeed("x");
etree::Element rss20Feed("x");


MU_SETUP(loadFeeds)
{
    atomFeed = etree::parse("testdata/pypy.atom.xml").getroot();
    rss20Feed = etree::parse("testdata/metafilter.rss.xml").getroot();
}


MU_TEST(atomFormat)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.format() == etree::feed::FORMAT_ATOM);
}


MU_TEST(rss20Format)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.format() == etree::feed::FORMAT_RSS20);
}


MU_TEST(atomTitle)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.title() == "PyPy Status Blog");
}


MU_TEST(rss20Title)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.title() == "MetaFilter");
}


MU_TEST(atomLink)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.link() == "http://morepypy.blogspot.com/");
}


MU_TEST(rss20Link)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.link() == "http://www.metafilter.com/");
}


MU_TEST(atomDescription)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.description() == "My subtitle");
}


MU_TEST(rss20Description)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.description() == "The past 24 hours of MetaFilter");
}


MU_TEST(atomIcon)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.icon() == "http://example.org/favicon.ico");
}


MU_TEST(rss20Icon)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.icon() == "http://www.lolcats.com/");
}


MU_TEST(atomItems)
{
    auto feed = etree::feed::fromelement(atomFeed);
    std::vector<std::string> got, expect {
        "C-API Support update",
        "Using CFFI for embedding",
    };

    for(auto &item : feed.items()) {
        got.push_back(item.title());
    }

    assert(got == expect);
}


MU_TEST(rss20Items)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    std::vector<std::string> got, expect {
		"Illinois Budget on hold",
		"Finger-lickin' 8-bit"
    };

    for(auto &item : feed.items()) {
        got.push_back(item.title());
    }

    assert(got == expect);
}


MU_TEST(atomElement)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.element() == atomFeed);
}


MU_TEST(rss20Element)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.element() == rss20Feed);
}


MU_TEST(atomCreate)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto s = tostring(feed.element());
    assert(s == "<feed xmlns=\"http://www.w3.org/2005/Atom\"/>");
}


MU_TEST(rss20Create)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto s = tostring(feed.element());
    assert(s == "<rss/>");
}


MU_TEST(atomAppend)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    assert(tostring(feed.element()) == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<entry>"
                "<title type=\"text\"/>"
                "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
                "<content type=\"html\"/>"
                "<author>"
                    "<name/>"
                "</author>"
                "<id/>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20Append)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    TOSTRING(feed.element());
    assert(tostring(feed.element()) == (
        "<rss>"
            "<entry>"
                "<title/>"
                "<link/>"
                "<link/>"
                "<guid/>"
            "</entry>"
        "</rss>"
    ));
}
