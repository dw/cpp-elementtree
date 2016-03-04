/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"


using std::pair;
using std::vector;
using std::string;

etree::Element atomFeed("x");
etree::Element rss20Feed("x");

TEST_CASE("loadFeeds", "[feed]")
{
    atomFeed = etree::parse("testdata/pypy.atom.xml").getroot();
    rss20Feed = etree::parse("testdata/metafilter.rss.xml").getroot();

//
// feed::format()
//


    SECTION("atomFormat")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.format() == etree::feed::FORMAT_ATOM);
    }

    SECTION("rss20Format")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.format() == etree::feed::FORMAT_RSS20);
    }


//
// feed::title()
//


    SECTION("atomTitle")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.title() == "PyPy Status Blog");
    }

    SECTION("rss20Title")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.title() == "MetaFilter");
    }


//
// feed::link()
//


    SECTION("atomLink")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.link() == "http://morepypy.blogspot.com/");
    }

    SECTION("rss20Link")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.link() == "http://www.metafilter.com/");
    }


//
// feed::description()
//


    SECTION("atomDescription")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.description() == "My subtitle");
    }

    SECTION("rss20Description")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.description() == "The past 24 hours of MetaFilter");
    }


//
// feed::icon()
//


    SECTION("atomIcon")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.icon() == "http://example.org/favicon.ico");
    }

    SECTION("atomIconSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        feed.icon("http://www.lolcats.com/");
        auto s = tostring(feed.element());
        REQUIRE(s == (
                "<feed xmlns=\"http://www.w3.org/2005/Atom\">"
                        "<icon>http://www.lolcats.com/</icon>"
                        "</feed>"
        ));
    }

    SECTION("rss20Icon")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.icon() == "http://www.lolcats.com/");
    }

    SECTION("rss20IconSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        feed.icon("http://www.lolcats.com/");
        feed.title("foo");
        feed.link("foobar");
        auto s = tostring(feed.element());
        REQUIRE(s == (
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


    SECTION("atomItems")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        std::vector<std::string> got, expect{
                "C-API Support update",
                "Using CFFI for embedding",
        };

        for (auto& item : feed.items()) {
            got.push_back(item.title());
        }

        REQUIRE(got == expect);
    }

    SECTION("rss20Items")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        std::vector<std::string> got, expect{
                "Illinois Budget on hold",
                "Finger-lickin' 8-bit"
        };

        for (auto& item : feed.items()) {
            got.push_back(item.title());
        }

        REQUIRE(got == expect);
    }


//
// feed::fromelement()
//


    SECTION("atomElement")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.element() == atomFeed);
    }

    SECTION("rss20Element")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.element() == rss20Feed);
    }


//
// feed::create()
//


    SECTION("atomCreate")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto s = tostring(feed.element());
        REQUIRE(s == "<feed xmlns=\"http://www.w3.org/2005/Atom\"/>");
    }

    SECTION("rss20Create")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto s = tostring(feed.element());
        REQUIRE(s == (
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


    SECTION("atomAppend")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("rss20Append")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("atomAppendExisting")
    {
        //
    }

    SECTION("rss20AppendExisting")
    {
        //
    }


//
// Item::title()
//


    SECTION("atomItemTitle")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        auto item = feed.items()[0];
        REQUIRE(item.title() == "C-API Support update");
    }

    SECTION("atomItemTitleSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.title("Example Title");
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("rss20ItemTitle")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        auto item = feed.items()[0];
        REQUIRE(item.title() == "Illinois Budget on hold");
    }

    SECTION("rss20ItemTitleSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.title("Example Title");
        REQUIRE(tostring(feed.element()) == (
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


    SECTION("atomItemLink")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        auto item = feed.items()[0];
        REQUIRE(item.link() == ("http://feedproxy.google.com/~r/PyPyStatusBlog/"
                "~3/S2p48K40LA8/c-api-support-update.html"));
    }

    SECTION("atomItemLinkSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.link("http://www.example.com/");
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("rss20ItemLink")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        auto item = feed.items()[0];
        REQUIRE(item.link()
                       == "http://www.metafilter.com/157514/Illinois-Budget-on-hold");
    }

    SECTION("rss20ItemLinkSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.link("http://www.example.com/");
        REQUIRE(tostring(feed.element()) == (
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


    SECTION("atomItemContent")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        auto item = feed.items()[0];
        std::string expect("<p>As you know, PyPy can emulate the");
        REQUIRE(expect == item.content().substr(0, expect.size()));
    }

    SECTION("atomItemContentSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.content("My content");
        item.type(etree::feed::CTYPE_HTML);
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("rss20ItemContent")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        auto item = feed.items()[0];
        std::string expect("<a href=\"http://interactive.wbez.org/");
        REQUIRE(item.content().substr(0, expect.size()) == expect);
    }

    SECTION("rss20ItemContentSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.content("http://www.example.com/");
        REQUIRE(tostring(feed.element()) == (
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


    SECTION("atomItemContentType")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.items()[0].type() == etree::feed::CTYPE_HTML);
    }

    SECTION("atomItemContentTypeSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.content("");

        item.type(etree::feed::CTYPE_HTML);
        REQUIRE(tostring(feed.element()) == (
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
        REQUIRE(tostring(feed.element()) == (
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

    SECTION("rss20ItemContentType")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.items()[0].type() == etree::feed::CTYPE_HTML);
        // ...
    }

    SECTION("rss20ItemContentTypeSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.type(etree::feed::CTYPE_HTML);
        // ...
    }


//
// Item::author()
//


    SECTION("atomItemAuthor")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.items()[0].author() == "Armin Rigo");
    }

    SECTION("atomItemAuthorSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.author("My Author");
        REQUIRE(tostring(item.element()) == (
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

    SECTION("rss20ItemAuthor")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.items()[0].author() == "AlexiaSky");
    }

    SECTION("rss20ItemAuthorSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.author("My Author");
        REQUIRE(tostring(item.element()) == (
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


    SECTION("atomItemGuid")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.items()[0].guid() == (
                "tag:blogger.com,1999:blog-3971202189709462152"
                        ".post-8582726091670983181"
        ));
    }

    SECTION("atomItemGuidSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.guid("x");
        REQUIRE(tostring(item.element()) == (
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

    SECTION("rss20ItemGuid")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.items()[0].guid() == "tag:metafilter.com,2016:site.157514");
    }

    SECTION("rss20ItemGuidSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.guid("x");
        REQUIRE(tostring(item.element()) == (
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


    SECTION("atomItemPublished")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.items()[0].published() == 1456415640);
    }

    SECTION("atomItemPublishedSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.published(1);
        REQUIRE(tostring(item.element()) == (
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

    SECTION("rss20ItemPublished")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.items()[0].published() == 1456713220);
    }

    SECTION("rss20ItemPublishedSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.published(1);
        REQUIRE(tostring(item.element()) == (
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


    SECTION("atomItemUpdated")
    {
        auto feed = etree::feed::fromelement(atomFeed);
        REQUIRE(feed.items()[0].updated() == 1456417492);
    }

    SECTION("atomItemUpdatedSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_ATOM);
        auto item = feed.append();
        item.updated(1);
        REQUIRE(tostring(item.element()) == (
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

    SECTION("rss20ItemUpdated")
    {
        auto feed = etree::feed::fromelement(rss20Feed);
        REQUIRE(feed.items()[0].updated() == 0);
    }

    SECTION("rss20ItemUpdatedSet")
    {
        auto feed = etree::feed::create(etree::feed::FORMAT_RSS20);
        auto item = feed.append();
        item.updated(1);
        REQUIRE(tostring(item.element()) == (
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

}
