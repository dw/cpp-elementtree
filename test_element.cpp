
#include <cassert>
#include <utility>
#include <vector>

#include "myunit.hpp"

#include "element.hpp"

using etree::Element;
using std::pair;
using std::vector;
using std::string;


static auto DOC = (
    "<who type='people' count='1' foo:x='true' xmlns:foo='urn:foo'>"
        "<person type='human'>"
            "<name>David</name>"
        "</person>"
    "</who>"
);

static auto NS_DOC = (
    "<foo:who xmlns:foo='urn:foo'>"
        "<foo:person foo:type='human'><name>David</name></foo:person>"
    "</foo:who>"
);


// ------------
// Constructors
// ------------

MU_TEST(destructor)
{
    Element e("x");
}


MU_TEST(kvList)
{
    Element e("x", {
        {"a", "b"},
        {"c", "d"}
    });

    assert(e.attrib().get("a") == "b");
    assert(! e.attrib().has("b"));
    assert(e.attrib().get("c") == "d");
    assert(! e.attrib().has("d"));
}


// ---------
// Accessors
// ---------

MU_TEST(qname)
{
    Element e("x");
    assert(e.qname() == "x");
}


MU_TEST(qnameNs)
{
    Element e("{urn:woah}x");
    assert(e.qname() == "{urn:woah}x");
}


MU_TEST(setQname)
{
    Element e("x");
    e.qname("y");
    assert(e.qname() == "y");
}


MU_TEST(setQnameNs)
{
    Element e("x");
    e.qname("{x}y");
    assert(e.qname() == "{x}y");
}


MU_TEST(tag)
{
    Element e("x");
    assert(e.tag() == "x");
}


MU_TEST(setTag)
{
    Element e("x");
    e.tag("y");
    assert(e.tag() == "y");
    assert(e.ns() == "");
}


MU_TEST(setTagKeepNs)
{
    Element e("{x}y");
    e.tag("z");
    assert(e.tag() == "z");
    assert(e.ns() == "x");
}


// -------
// AttrMap
// -------


// AttrIterator
MU_TEST(attrIter)
{
    vector<pair<string, string>> got, expect {
        { "type", "people" },
        { "count", "1" },
        { "{urn:foo}x", "true" },
    };

    auto root = etree::fromstring(DOC);
    for(auto attr : root.attrib()) {
        got.emplace_back(attr.qname().tostring(), attr.value());
    }
    assert(got == expect);
}


MU_TEST(attrHas)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().has("type"));
    assert(! root.attrib().has("missing"));
}


MU_TEST(attrGet)
{
    auto root = etree::fromstring(DOC);
    assert("people" == root.attrib().get("type"));
    assert("" == root.attrib().get("x"));
    assert("true" == root.attrib().get("{urn:foo}x"));
}


MU_TEST(attrGetDefault)
{
    auto root = etree::fromstring(DOC);
    assert("people" == root.attrib().get("type", "default"));
    assert("default" == root.attrib().get("x", "default"));
    assert("true" == root.attrib().get("{urn:foo}x", "default"));
}


MU_TEST(attrSetNoExist)
{
    auto e = Element("a");
    e.attrib().set("a", "b");
    assert("b" == e.attrib().get("a"));
}


MU_TEST(attrSetNs)
{
    auto e = Element("a");
    e.attrib().set("{x}y", "1");
    assert("1" == e.attrib().get("{x}y"));
}


MU_TEST(attrKeys)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().keys() == vector<etree::QName>({
        "type",
        "count",
        "{urn:foo}x"
    }));
}


MU_TEST(attrKeysEmpty)
{
    auto e = Element("a");
    assert(e.attrib().keys() == vector<etree::QName> {});
}


MU_TEST(attrRemove)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().remove("type"));
    assert(! root.attrib().remove("type"));
    assert(! root.attrib().has("type"));
}


MU_TEST(attrRemoveNs)
{
    auto root = etree::fromstring(DOC);
    assert(root.attrib().remove("{urn:foo}x"));
    assert(! root.attrib().remove("{urn:foo}x"));
    assert(! root.attrib().has("{urn:foo}x"));
}


MU_TEST(attrRemoveEmpty)
{
    auto e = Element("a");
    assert(! e.attrib().remove("x"));
}


MU_TEST(testSize)
{
    Element e("x");

    assert(e.attrib().size() == 0);
    e.attrib().set("a", "b");
    assert(e.attrib().size() == 1);

    e.attrib().remove("a");
    assert(e.attrib().size() == 0);
}



// ---
// Rest??
// ---

MU_TEST(getNoNs)
{
    auto root = etree::fromstring(DOC);
    assert("human" == (*root.child("person")).get("type"));
}


MU_TEST(getNs)
{
    auto root = etree::fromstring(NS_DOC);
    #define NS "{urn:foo}"
    assert("human" == (*root.child(NS "person")).get(NS "type"));
}

MU_MAIN()
