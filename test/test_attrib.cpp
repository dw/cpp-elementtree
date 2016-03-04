/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"

#include "test_consts.hpp"

using std::pair;
using std::vector;
using std::string;


TEST_CASE("Has", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE(root.attrib().has("type"));
    REQUIRE_FALSE(root.attrib().has("missing"));
}


TEST_CASE("Get", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE("people" == root.attrib().get("type"));
    REQUIRE("" == root.attrib().get("x"));
    REQUIRE("true" == root.attrib().get("{urn:ns}x"));
}


TEST_CASE("GetDefault", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE("people" == root.attrib().get("type", "default"));
    REQUIRE("default" == root.attrib().get("x", "default"));
    REQUIRE("true" == root.attrib().get("{urn:ns}x", "default"));
}


TEST_CASE("SetNoExist", "[attrib]")
{
    auto e = etree::Element("a");
    e.attrib().set("a", "b");
    REQUIRE("b" == e.attrib().get("a"));
}


TEST_CASE("SetNs", "[attrib]")
{
    auto e = etree::Element("a");
    e.attrib().set("{x}y", "1");
    REQUIRE("1" == e.attrib().get("{x}y"));
}


TEST_CASE("SetKv", "[attrib]")
{
    auto e = etree::Element("a");
    e.attrib().set({
        {"x", "1"},
        {"y", "2"}
    });
    REQUIRE(e.attrib().size() == 2);
    REQUIRE("1" == e.attrib().get("x"));
    REQUIRE("2" == e.attrib().get("y"));
}


TEST_CASE("Keys", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE(root.attrib().keys() == vector<etree::QName>({
        "type",
        "count",
        "{urn:ns}x"
    }));
}


TEST_CASE("KeysEmpty", "[attrib]")
{
    auto e = etree::Element("a");
    REQUIRE(e.attrib().keys() == vector<etree::QName> {});
}


TEST_CASE("Remove", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE(root.attrib().remove("type"));
    REQUIRE_FALSE(root.attrib().remove("type"));
    REQUIRE_FALSE(root.attrib().has("type"));
}


TEST_CASE("RemoveNs", "[attrib]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE(root.attrib().remove("{urn:ns}x"));
    REQUIRE_FALSE(root.attrib().remove("{urn:ns}x"));
    REQUIRE_FALSE(root.attrib().has("{urn:ns}x"));
}


TEST_CASE("RemoveEmpty", "[attrib]")
{
    auto e = etree::Element("a");
    REQUIRE_FALSE(e.attrib().remove("x"));
}


TEST_CASE("size", "[attrib]")
{
    etree::Element e("x");

    REQUIRE(e.attrib().size() == 0);
    e.attrib().set("a", "b");
    REQUIRE(e.attrib().size() == 1);

    e.attrib().remove("a");
    REQUIRE(e.attrib().size() == 0);
}


//
// AttrIterator
//


TEST_CASE("iter", "[attrib]")
{
    vector<pair<string, string>> got, expect {
        { "type", "people" },
        { "count", "1" },
        { "{urn:ns}x", "true" },
    };

    auto root = etree::fromstring(DOC);
    for(auto attr : root.attrib()) {
        got.emplace_back(attr.qname().tostring(), attr.value());
    }
    REQUIRE(got == expect);
}


TEST_CASE("iterSurvivesMutation", "[attrib]")
{
    auto root = etree::fromstring("<a a=\"1\" b=\"2\" c=\"3\"/>");
    std::vector<std::string> got, expect {
        "b",
        "c"
    };

    for(auto attr : root.attrib()) {
        if(attr.tag() == "a") {
            root.attrib().remove("a");
        } else {
            got.push_back(attr.tag());
        }
    }

    REQUIRE(got == expect);
    REQUIRE(etree::tostring(root) == "<a b=\"2\" c=\"3\"/>");
}
