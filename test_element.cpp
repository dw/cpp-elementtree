
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
    "<who xmlns:foo=\"urn:foo\" type=\"people\" count=\"1\" foo:x=\"true\">"
        "<person type=\"human\">"
            "<name foo:attrx=\"3\">David</name>"
            "<foo:attr1>123</foo:attr1>"
            "<foo:attr2>123</foo:attr2>"
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


// ------
// append
// ------


MU_TEST(appendSelfFails)
{
    auto root = etree::fromstring(DOC);
    MU_RAISES(etree::cyclical_tree_error, [&]() {
        root.append(root);
    })
}


MU_TEST(appendAncestorFails)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    MU_RAISES(etree::cyclical_tree_error, [&]() {
        person.append(root);
    })
}


MU_TEST(appendNew)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    assert(root.size() == 1);
    assert(child == *root.child("child"));
}


MU_TEST(appendNewTwice)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    root.append(child);
    assert(root.size() == 1);
    assert(child == *root.child("child"));
}


MU_TEST(appendDuplicateNs)
{
    auto root = etree::fromstring(DOC);
    auto child = etree::Element("{urn:foo}bar");
    child.attrib().set("{urn:foo}baz", "1");
    root.append(child);
    assert(etree::tostring(child) == "<foo:bar foo:baz=\"1\"/>");
}


// ------
// remove
// ------


MU_TEST(removeNoArg)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    assert(! person.getparent());
    assert(! root.child("person"));
}


MU_TEST(removeArg)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    assert(! person.getparent());
    assert(! root.child("person"));
}


MU_TEST(removeArgNotParent)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    root.remove(name);
    assert(root.size() == 1);
    assert((*name.getparent()).tag() == "person");
    assert(! root.child("name"));
}


MU_TEST(removeTwiceNoArgs)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    person.remove();
    assert(! root.child("person"));
}


MU_TEST(removeSucceeds)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
}


MU_TEST(removeTwiceOkay)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.remove(person);
}


MU_TEST(removeThenAppend)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.append(person);
    assert(DOC == etree::tostring(root));
}


MU_TEST(removeNsPreserved)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    assert(etree::tostring(name) ==
           "<name xmlns:ns0=\"urn:foo\" ns0:attrx=\"3\">David</name>");
}


MU_TEST(removeAddNsCollapsed)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    root.append(name);
    assert(etree::tostring(name) ==
           "<name foo:attrx=\"3\">David</name>");
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
