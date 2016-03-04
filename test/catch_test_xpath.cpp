/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"


TEST_CASE("contextConstructorNoNs", "[xpath]")
{
    etree::XPathContext ctx;
}


TEST_CASE("contextConstructor", "[xpath]")
{
    etree::XPathContext ctx(etree::ns_list{
      {"foo", "urn:foo"}
    });
}


TEST_CASE("contextNsListRespected", "[xpath]")
{
    auto elem = etree::fromstring(
        "<root><child xmlns=\"urn:foo\"/></root>"
    );
    etree::XPathContext ctx(etree::ns_list{
        {"foo", "urn:foo"}
    });

    auto expr = etree::XPath("child", ctx);
    REQUIRE(expr.findall(elem).size() == 0);

    auto expr2 = etree::XPath("foo:child", ctx);
    REQUIRE(expr2.findall(elem).size() == 1);
}


TEST_CASE("XPathConstructor", "[xpath]")
{
    auto xp = etree::XPath(".");
    REQUIRE(xp.expr() == ".");
}


TEST_CASE("ConstructorParseError", "[xpath]")
{
    REQUIRE_THROWS_AS(etree::XPath("&%^&%^&"), etree::xml_error);
    //REQUIRE(e.what() == std::string("Invalid expression"));
}


TEST_CASE("CopyConstructor", "[xpath]")
{
    auto xp = etree::XPath(".");
    auto xp2 = xp;
    REQUIRE(xp.expr() == xp2.expr());
}


TEST_CASE("Expr", "[xpath]")
{
    auto xp = etree::XPath(".");
    REQUIRE(xp.expr() == ".");
}


TEST_CASE("FindOrder", "[xpath]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    REQUIRE(elem.child("a") == xp.find(elem));
}


TEST_CASE("FindNoMatch", "[xpath]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    REQUIRE_FALSE(xp.find(elem));
}


TEST_CASE("Findall", "[xpath]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    REQUIRE(elem.children() == xp.findall(elem));
}


TEST_CASE("FindallNoMatch", "[xpath]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    REQUIRE(elem.children("nonexistent") == xp.findall(elem));
}


TEST_CASE("removeall", "[xpath]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./b");
    auto removed = xp.removeall(elem);
    REQUIRE(removed.size() == 1);
    REQUIRE_FALSE(removed[0].getparent());
    REQUIRE(elem.children("b").size() == 0);
}


TEST_CASE("FindText", "[xpath]")
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("name");
    REQUIRE("David" == xp.findtext(elem));
}


TEST_CASE("FindTextDefault", "[xpath]")
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("age");
    REQUIRE("Unknown" == xp.findtext(elem, "Unknown"));
}
