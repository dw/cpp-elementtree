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


//
// feed::format()
//


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


//
// feed::title()
//


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


//
// feed::link()
//


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


//
// feed::description()
//


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


//
// feed::icon()
//


MU_TEST(atomIcon)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.icon() == "http://example.org/favicon.ico");
}


MU_TEST(atomIconSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    feed.icon("http://www.lolcats.com/");
    auto s = tostring(feed.element());
    assert(s == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<icon>http://www.lolcats.com/</icon>"
        "</feed>"
    ));
}


MU_TEST(rss20Icon)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.icon() == "http://www.lolcats.com/");
}


MU_TEST(rss20IconSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    feed.icon("http://www.lolcats.com/");
    feed.title("foo");
    feed.link("foobar");
    auto s = tostring(feed.element());
    assert(s == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel>"
                "<image>"
                    "<title>foo</title>"
                    "<link>foobar</link>"
                    "<url>http://www.lolcats.com/</url>"
                "</image>"
                "<title>foo</title>"
                "<link>foobar</link>"
            "</channel>"
        "</rss>"
    ));
}


//
// feed::items()
//


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


//
// feed::fromelement()
//


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


//
// feed::create()
//


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
    assert(s == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel/>"
        "</rss>"
    ));
}


//
// Feed::append()
//


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
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20Append)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    assert(tostring(feed.element()) == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel>"
                "<item>"
                    "<title/>"
                    "<link/>"
                    "<ns0:creator/>"
                    "<guid isPermaLink=\"false\"/>"
                    "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
                    "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
                "</item>"
            "</channel>"
        "</rss>"
    ));
}


MU_TEST(atomAppendExisting)
{
    //
}


MU_TEST(rss20AppendExisting)
{
    //
}


//
// Item::title()
//


MU_TEST(atomItemTitle)
{
    auto feed = etree::feed::fromelement(atomFeed);
    auto item = feed.items()[0];
    assert(item.title() == "C-API Support update");
}


MU_TEST(atomItemTitleSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.title("Example Title");
    assert(tostring(feed.element()) == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<entry>"
                "<title type=\"text\">Example Title</title>"
                "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
                "<content type=\"html\"/>"
                "<author>"
                    "<name/>"
                "</author>"
                "<id/>"
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20ItemTitle)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    auto item = feed.items()[0];
    assert(item.title() == "Illinois Budget on hold");
}


MU_TEST(rss20ItemTitleSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.title("Example Title");
    assert(tostring(feed.element()) == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel>"
                "<item>"
                    "<title>Example Title</title>"
                    "<link/>"
                    "<ns0:creator/>"
                    "<guid isPermaLink=\"false\"/>"
                    "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
                    "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
                "</item>"
            "</channel>"
        "</rss>"
    ));
}


//
// Item::link()
//


MU_TEST(atomItemLink)
{
    auto feed = etree::feed::fromelement(atomFeed);
    auto item = feed.items()[0];
    assert(item.link() == ("http://feedproxy.google.com/~r/PyPyStatusBlog/"
                           "~3/S2p48K40LA8/c-api-support-update.html"));
}


MU_TEST(atomItemLinkSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.link("http://www.example.com/");
    assert(tostring(feed.element()) == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<entry>"
                "<title type=\"text\"/>"
                "<link rel=\"alternate\" type=\"text/html\" "
                    "href=\"http://www.example.com/\"/>"
                "<content type=\"html\"/>"
                "<author>"
                    "<name/>"
                "</author>"
                "<id/>"
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20ItemLink)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    auto item = feed.items()[0];
    assert(item.link() == "http://www.metafilter.com/157514/Illinois-Budget-on-hold");
}


MU_TEST(rss20ItemLinkSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.link("http://www.example.com/");
    assert(tostring(feed.element()) == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel>"
                "<item>"
                    "<title/>"
                    "<link>http://www.example.com/</link>"
                    "<ns0:creator/>"
                    "<guid isPermaLink=\"false\"/>"
                    "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
                    "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
                "</item>"
            "</channel>"
        "</rss>"
    ));
}


//
// Item::content()
//


MU_TEST(atomItemContent)
{
    auto feed = etree::feed::fromelement(atomFeed);
    auto item = feed.items()[0];
    std::string expect("<p>As you know, PyPy can emulate the");
    assert(expect == item.content().substr(0, expect.size()));
}


MU_TEST(atomItemContentSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.content("My content");
    item.type(etree::feed::CTYPE_HTML);
    assert(tostring(feed.element()) == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<entry>"
                "<title type=\"text\"/>"
                "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
                "<content type=\"html\">My content</content>"
                "<author>"
                    "<name/>"
                "</author>"
                "<id/>"
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20ItemContent)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    auto item = feed.items()[0];
    std::string expect("<a href=\"http://interactive.wbez.org/");
    assert(item.content().substr(0, expect.size()) == expect);
}


MU_TEST(rss20ItemContentSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.content("http://www.example.com/");
    assert(tostring(feed.element()) == (
        "<rss xmlns:ns0=\"http://purl.org/dc/elements/1.1/\" "
                "xmlns:ns1=\"http://www.w3.org/2005/Atom\" "
                "version=\"2.0\">"
            "<channel>"
                "<item>"
                    "<title/>"
                    "<link/>"
                    "<ns0:creator/>"
                    "<guid isPermaLink=\"false\"/>"
                    "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
                    "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
                    "<description>http://www.example.com/</description>"
                "</item>"
            "</channel>"
        "</rss>"
    ));
}


//
// Item::content_type()
//


MU_TEST(atomItemContentType)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.items()[0].type() == etree::feed::CTYPE_HTML);
}


MU_TEST(atomItemContentTypeSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.content("");

    item.type(etree::feed::CTYPE_HTML);
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
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));

    item.type(etree::feed::CTYPE_TEXT);
    assert(tostring(feed.element()) == (
        "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
            "<entry>"
                "<title type=\"text\"/>"
                "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
                "<content type=\"text\"/>"
                "<author>"
                    "<name/>"
                "</author>"
                "<id/>"
                "<published>1970-01-01T00:00:00Z</published>"
                "<updated>1970-01-01T00:00:00Z</updated>"
            "</entry>"
        "</feed>"
    ));
}


MU_TEST(rss20ItemContentType)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.items()[0].type() == etree::feed::CTYPE_HTML);
    // ...
}


MU_TEST(rss20ItemContentTypeSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.type(etree::feed::CTYPE_HTML);
    // ...
}


//
// Item::author()
//


MU_TEST(atomItemAuthor)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.items()[0].author() == "Armin Rigo");
}


MU_TEST(atomItemAuthorSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.author("My Author");
    assert(tostring(item.element()) == (
        "<entry>"
            "<title type=\"text\"/>"
            "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
            "<content type=\"html\"/>"
            "<author>"
                "<name>My Author</name>"
            "</author>"
            "<id/>"
            "<published>1970-01-01T00:00:00Z</published>"
            "<updated>1970-01-01T00:00:00Z</updated>"
        "</entry>"
    ));
}


MU_TEST(rss20ItemAuthor)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.items()[0].author() == "AlexiaSky");
}


MU_TEST(rss20ItemAuthorSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.author("My Author");
    assert(tostring(item.element()) == (
        "<item>"
            "<title/>"
            "<link/>"
            "<ns0:creator>My Author</ns0:creator>"
            "<guid isPermaLink=\"false\"/>"
            "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
            "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
        "</item>"
    ));
}


//
// Item::guid()
//


MU_TEST(atomItemGuid)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.items()[0].guid() == (
        "tag:blogger.com,1999:blog-3971202189709462152"
        ".post-8582726091670983181"
    ));
}


MU_TEST(atomItemGuidSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.guid("x");
    assert(tostring(item.element()) == (
        "<entry>"
            "<title type=\"text\"/>"
            "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
            "<content type=\"html\"/>"
            "<author>"
                "<name/>"
            "</author>"
            "<id>x</id>"
            "<published>1970-01-01T00:00:00Z</published>"
            "<updated>1970-01-01T00:00:00Z</updated>"
        "</entry>"
    ));
}


MU_TEST(rss20ItemGuid)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.items()[0].guid() == "tag:metafilter.com,2016:site.157514");
}


MU_TEST(rss20ItemGuidSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.guid("x");
    assert(tostring(item.element()) == (
        "<item>"
            "<title/>"
            "<link/>"
            "<ns0:creator/>"
            "<guid isPermaLink=\"false\">x</guid>"
            "<pubDate>Thu, 01 Jan 1970 00:00:00 +0000</pubDate>"
            "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
        "</item>"
    ));
}


//
// Item::published()
//


MU_TEST(atomItemPublished)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.items()[0].published() == 1456415640);
}


MU_TEST(atomItemPublishedSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.published(1);
    assert(tostring(item.element()) == (
        "<entry>"
            "<title type=\"text\"/>"
            "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
            "<content type=\"html\"/>"
            "<author>"
                "<name/>"
            "</author>"
            "<id/>"
            "<published>1970-01-01T00:00:01Z</published>"
            "<updated>1970-01-01T00:00:00Z</updated>"
        "</entry>"
    ));
}


MU_TEST(rss20ItemPublished)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.items()[0].published() == 1456713220);
}


MU_TEST(rss20ItemPublishedSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.published(1);
    assert(tostring(item.element()) == (
        "<item>"
            "<title/>"
            "<link/>"
            "<ns0:creator/>"
            "<guid isPermaLink=\"false\"/>"
            "<pubDate>Thu, 01 Jan 1970 00:00:01 +0000</pubDate>"
            "<ns1:updated>1970-01-01T00:00:00Z</ns1:updated>"
        "</item>"
    ));
}


//
// Item::updated()
//


MU_TEST(atomItemUpdated)
{
    auto feed = etree::feed::fromelement(atomFeed);
    assert(feed.items()[0].updated() == 1456417492);
}


MU_TEST(atomItemUpdatedSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
    auto item = feed.append();
    item.updated(1);
    assert(tostring(item.element()) == (
        "<entry>"
            "<title type=\"text\"/>"
            "<link rel=\"alternate\" type=\"text/html\" href=\"\"/>"
            "<content type=\"html\"/>"
            "<author>"
                "<name/>"
            "</author>"
            "<id/>"
            "<published>1970-01-01T00:00:00Z</published>"
            "<updated>1970-01-01T00:00:01Z</updated>"
        "</entry>"
    ));
}


MU_TEST(rss20ItemUpdated)
{
    auto feed = etree::feed::fromelement(rss20Feed);
    assert(feed.items()[0].updated() == 0);
}


MU_TEST(rss20ItemUpdatedSet)
{
    auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
    auto item = feed.append();
    item.updated(1);
    assert(tostring(item.element()) == (
        "<item>"
            "<title/>"
            "<link/>"
            "<ns0:creator/>"
            "<guid isPermaLink=\"false\"/>"
            "<pubDate>Thu, 01 Jan 1970 00:00:01 +0000</pubDate>"
            "<ns1:updated>1970-01-01T00:00:01Z</ns1:updated>"
        "</item>"
    ));
}
