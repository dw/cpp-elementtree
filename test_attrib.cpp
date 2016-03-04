/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <utility>
#include <vector>

#include "myunit.hpp"
#include "element.hpp"
#include "test_consts.hpp"

using std::pair;
using std::vector;
using std::string;


MU_TEST(Has)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().has("type"));
    assert(! root.attrib().has("missing"));
}


MU_TEST(Get)
{
    auto root = etree::fromstring(DOC);
    assert("people" == root.attrib().get("type"));
    assert("" == root.attrib().get("x"));
    assert("true" == root.attrib().get("{urn:ns}x"));
}


MU_TEST(GetDefault)
{
    auto root = etree::fromstring(DOC);
    assert("people" == root.attrib().get("type", "default"));
    assert("default" == root.attrib().get("x", "default"));
    assert("true" == root.attrib().get("{urn:ns}x", "default"));
}


MU_TEST(SetNoExist)
{
    auto e = etree::Element("a");
    e.attrib().set("a", "b");
    assert("b" == e.attrib().get("a"));
}


MU_TEST(SetNs)
{
    auto e = etree::Element("a");
    e.attrib().set("{x}y", "1");
    assert("1" == e.attrib().get("{x}y"));
}


MU_TEST(SetKv)
{
    auto e = etree::Element("a");
    e.attrib().set({
        {"x", "1"},
        {"y", "2"}
    });
    assert(e.attrib().size() == 2);
    assert("1" == e.attrib().get("x"));
    assert("2" == e.attrib().get("y"));
}


MU_TEST(Keys)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().keys() == vector<etree::QName>({
        "type",
        "count",
        "{urn:ns}x"
    }));
}


MU_TEST(KeysEmpty)
{
    auto e = etree::Element("a");
    assert(e.attrib().keys() == vector<etree::QName> {});
}


MU_TEST(Remove)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().remove("type"));
    assert(! root.attrib().remove("type"));
    assert(! root.attrib().has("type"));
}


MU_TEST(RemoveNs)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().remove("{urn:ns}x"));
    assert(! root.attrib().remove("{urn:ns}x"));
    assert(! root.attrib().has("{urn:ns}x"));
}


MU_TEST(RemoveEmpty)
{
    auto e = etree::Element("a");
    assert(! e.attrib().remove("x"));
}


MU_TEST(size)
{
    etree::Element e("x");

    assert(e.attrib().size() == 0);
    e.attrib().set("a", "b");
    assert(e.attrib().size() == 1);

    e.attrib().remove("a");
    assert(e.attrib().size() == 0);
}


//
// AttrIterator
//


MU_TEST(iter)
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
    assert(got == expect);
}


MU_TEST(iterSurvivesMutation)
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

    assert(got == expect);
    assert(etree::tostring(root) == "<a b=\"2\" c=\"3\"/>");
}
