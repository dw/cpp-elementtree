
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

#define OUT(x) std::cout << x << std::endl;
#define TOSTRING(x) OUT(etree::tostring(x))


// ------------
// Constructors
// ------------


MU_TEST(elemDestructor)
{
    Element e("x");
}


MU_TEST(elemKvList)
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


MU_TEST(elemQname)
{
    Element e("x");
    assert(e.qname() == "x");
}


MU_TEST(elemQnameNs)
{
    Element e("{urn:woah}x");
    assert(e.qname() == "{urn:woah}x");
}


MU_TEST(elemQnameSet)
{
    Element e("x");
    e.qname("y");
    assert(e.qname() == "y");
}


MU_TEST(elemQnameSetNs)
{
    Element e("x");
    e.qname("{x}y");
    assert(e.qname() == "{x}y");
}


MU_TEST(elemTag)
{
    Element e("x");
    assert(e.tag() == "x");
}


MU_TEST(elemTagSet)
{
    Element e("x");
    e.tag("y");
    assert(e.tag() == "y");
    assert(e.ns() == "");
}


MU_TEST(elemTagSetKeepNs)
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


MU_TEST(attrSetKv)
{
    auto e = Element("a");
    e.attrib().set({
        {"x", "1"},
        {"y", "2"}
    });
    assert(e.attrib().size() == 2);
    assert("1" == e.attrib().get("x"));
    assert("2" == e.attrib().get("y"));
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


// ----------
// ancestorOf
// ----------


MU_TEST(elemAncestorOfTrue)
{
    auto root = etree::fromstring("<a><b/></a>");
    assert(root.ancestorOf(*root.child("b")));
}


MU_TEST(elemAncestorOfFalse)
{
    auto root = etree::fromstring("<a><b/></a>");
    assert(! (*root.child("b")).ancestorOf(root));
}


MU_TEST(elemAncestorOfFalseWrongDoc)
{
    auto root = etree::fromstring("<a><b/></a>");
    auto root2 = etree::fromstring("<a><b/></a>");
    assert(! root.ancestorOf(root2));
}



// ------
// append
// ------


MU_TEST(elemAppendSelfFails)
{
    auto root = etree::fromstring(DOC);
    MU_RAISES(etree::cyclical_tree_error, [&]() {
        root.append(root);
    })
}


MU_TEST(elemAppendAncestorFails)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    MU_RAISES(etree::cyclical_tree_error, [&]() {
        person.append(root);
    })
}


MU_TEST(elemAppendNew)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    assert(root.size() == 1);
    assert(child == *root.child("child"));
}


MU_TEST(elemAppendNewTwice)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    root.append(child);
    assert(root.size() == 1);
    assert(child == *root.child("child"));
}


MU_TEST(elemAppendDuplicateNs)
{
    auto root = etree::fromstring(DOC);
    auto child = etree::Element("{urn:foo}bar");
    child.attrib().set("{urn:foo}baz", "1");
    root.append(child);
    assert(etree::tostring(child) == "<foo:bar foo:baz=\"1\"/>");
}


MU_TEST(elemAppendMoveNsSimple1)
{
    auto root = etree::fromstring("<a xmlns:foo=\"urn:foo\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:foo\"/>");
    root.append(root2);
    assert(etree::tostring(root) == "<a xmlns:foo=\"urn:foo\"><foo:b/></a>");
}


MU_TEST(elemAppendMoveNsSimple2)
{
    auto root = etree::fromstring("<a xmlns=\"urn:foo\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:foo\"/>");
    root.append(root2);
    assert(etree::tostring(root) == "<a xmlns=\"urn:foo\"><b/></a>");
}


MU_TEST(elemAppendMoveNsNested)
{
    auto root = etree::fromstring(DOC);
    auto root2 = etree::fromstring(DOC);
    root.append(*root2.find("person/name"));
    TOSTRING(root)
}


// ------
// remove
// ------


MU_TEST(elemRemoveNoArg)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    assert(! person.getparent());
    assert(! root.child("person"));
}


MU_TEST(elemRemoveArg)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    assert(! person.getparent());
    assert(! root.child("person"));
}


MU_TEST(elemRemoveArgNotParent)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    root.remove(name);
    assert(root.size() == 1);
    assert((*name.getparent()).tag() == "person");
    assert(! root.child("name"));
}


MU_TEST(elemRemoveTwiceNoArgs)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    person.remove();
    assert(! root.child("person"));
}


MU_TEST(elemRemoveSucceeds)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
}


MU_TEST(elemRemoveTwiceOkay)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.remove(person);
}


MU_TEST(elemRemoveThenAppend)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.append(person);
    assert(DOC == etree::tostring(root));
}


MU_TEST(elemRemoveNsPreserved)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    assert(etree::tostring(name) ==
           "<name xmlns:ns0=\"urn:foo\" ns0:attrx=\"3\">David</name>");
}


MU_TEST(elemRemoveAddNsCollapsed)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    root.append(name);
    assert(etree::tostring(name) ==
           "<name foo:attrx=\"3\">David</name>");
}


MU_TEST(elemRemovePreservesTail)
{
    auto elem = etree::Element("person");
    auto e2 = etree::SubElement(elem, "name");
    e2.tail("\n\n");
    e2.remove();
    elem.append(e2);
    assert(etree::tostring(elem) == (
        "<person><name/>\n"
        "\n"
        "</person>"
    ));
}


MU_TEST(elemRemovePreservesTailTextOnly)
{
    auto elem = etree::fromstring("<a><b/><c/></a>");
    auto e2 = *elem.child("b");
    e2.remove();
    assert(etree::tostring(elem) == "<a><c/></a>");
}


MU_TEST(elemText)
{
    auto elem = etree::fromstring("<name>David</name>");
    assert(elem.text() == "David");
}


MU_TEST(elemTextSet)
{
    auto elem = etree::fromstring("<name/>");
    elem.text("David");
    assert("<name>David</name>" == etree::tostring(elem));
}


MU_TEST(elemTextSetEmpty)
{
    auto elem = etree::fromstring("<name>David</name>");
    elem.text("");
    assert("<name/>" == etree::tostring(elem));
}


MU_TEST(elemTextSetChildElements)
{
    auto elem = etree::fromstring("<name><lang/></name>");
    elem.text("David");
    assert("<name>David<lang/></name>" == etree::tostring(elem));
}


//
// fromstring
//

MU_TEST(elemFromstringBadXml)
{
    MU_RAISES2(etree::xml_error,
        [&]() {
            etree::fromstring("corrupt");
        },
        [&](etree::xml_error &e) {
            auto expect = "Start tag expected, '<' not found\n";
            assert(e.what() == std::string(expect));
        }
    );
}


//
// tostring
//

MU_TEST(elemTostring)
{
    auto elem = etree::Element("name");
    elem.text("David");
    elem.attrib().set({
        {"{urn:foo}x", "1"},
        {"{urn:bar}y", "2"}
    });

    auto got = etree::tostring(elem);
    auto expect = ("<name xmlns:ns0=\"urn:foo\" xmlns:ns1=\"urn:bar\" "
                  "ns0:x=\"1\" ns1:y=\"2\">David</name>");
    assert(got == expect);
}


MU_TEST(treeTostring)
{
    auto elem = etree::Element("name");
    elem.text("David");
    elem.attrib().set({
        {"{urn:foo}x", "1"},
        {"{urn:bar}y", "2"}
    });

    auto got = etree::tostring(elem.getroottree());
    auto expect = ("<?xml version=\"1.0\"?>\n"
                   "<name xmlns:ns0=\"urn:foo\" xmlns:ns1=\"urn:bar\" "
                    "ns0:x=\"1\" ns1:y=\"2\">David</name>\n");
    assert(got == expect);
}



//
// QName
//

MU_TEST(qnameConstructNsTag)
{
    auto qn = etree::QName("ns", "tag");
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(qnameConstructCopy)
{
    auto qn = etree::QName("ns", "tag");
    auto qn2 = qn;
    assert(qn2.ns() == "ns");
    assert(qn2.tag() == "tag");
}


MU_TEST(qnameConstructUniversalName)
{
    auto qn = etree::QName(std::string("{ns}tag"));
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(qnameConstructUniversalNameChar)
{
    auto qn = etree::QName("{ns}tag");
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(qnameTostringNoNs)
{
    auto qn = etree::QName("nons");
    assert(qn.tostring() == "nons");
}


MU_TEST(qnameTostringNs)
{
    auto qn = etree::QName("{urn:foo}nons");
    assert(qn.tostring() == "{urn:foo}nons");
}


MU_TEST(qnameEquals)
{
    auto qn = etree::QName("{urn:foo}nons");
    assert(qn.equals("urn:foo", "nons"));
}


MU_TEST(qnameEqualsFalse)
{
    auto qn = etree::QName("{urn:foo}nons");
    assert(! qn.equals("urn:foo", "ns"));
}


MU_TEST(qnameEqualsFalseNoNs)
{
    auto qn = etree::QName("{urn:foo}nons");
    assert(! qn.equals(NULL, "nons"));
}


MU_TEST(qnameEqualsFalseWrongTag)
{
    auto qn = etree::QName("{urn:foo}nons");
    assert(! qn.equals("urn:foo", "ns"));
}


MU_TEST(qnameOpEqTrue)
{
    auto qn = etree::QName("{urn:foo}nons");
    auto qn2 = etree::QName("{urn:foo}nons");
    assert(qn == qn2);
}


MU_TEST(qnameOpFalseUnequalNs)
{
    auto qn = etree::QName("{urn:foo}nons");
    auto qn2 = etree::QName("{urn:foo2}nons");
    assert(qn != qn2);
}


MU_TEST(qnameOpEqFalseUnequalTag)
{
    auto qn = etree::QName("{urn:foo}nons");
    auto qn2 = etree::QName("{urn:foo}ns");
    assert(qn != qn2);
}


MU_TEST(qnaneOpUnequalMissingNs)
{
    auto qn = etree::QName("nons");
    auto qn2 = etree::QName("{urn:foo2}nons");
    assert(qn != qn2);
}


// ---
// Rest??
// ---


MU_TEST(elemGetNoNs)
{
    auto root = etree::fromstring(DOC);
    assert("human" == (*root.child("person")).get("type"));
}


MU_TEST(elemGetNs)
{
    auto root = etree::fromstring(NS_DOC);
    #define NS "{urn:foo}"
    assert("human" == (*root.child(NS "person")).get(NS "type"));
}


//
// Nullable
//


MU_TEST(nullableDefaultConstructor)
{
    auto val = etree::Nullable<etree::Element>();
    assert(! val);
}


MU_TEST(nullableConstructor)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    assert(val);
    assert(elem == *val);
}


MU_TEST(nullableRvalConstructor)
{
    auto val = etree::Nullable<etree::Element>(etree::Element("a"));
    assert(val);
    assert((*val).tag() == "a");
}


MU_TEST(nullableCopyConstructorUnset)
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = val;
    assert(! val);
    assert(! val2);
    assert(val == val2);
}


MU_TEST(nullableCopyConstructorSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = val;
    assert(val);
    assert(val2);
    assert(val == val2);
}


MU_TEST(nullableAssignUnsetToUnset)
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    assert(! val);
    assert(! val2);
}


MU_TEST(nullableAssignSetToUnset)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    assert(val);
    assert(val2);
    assert(*val == *val2);
}


MU_TEST(nullableAssignUnsetToSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>(elem);
    val2 = val;
    assert(! val);
    assert(! val2);
}


MU_TEST(nullableDerefUnset)
{
    auto val = etree::Nullable<etree::Element>();
    MU_RAISES(etree::missing_value_error, [&]() {
        *val;
    })
}


MU_TEST(nullableDerefSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    assert(*val == elem);
}


//
// XPath
//


MU_TEST(xpathConstructor)
{
    auto xp = etree::XPath(".");
    assert(xp.expr() == ".");
}


MU_TEST(xpathConstructorParseError)
{
    MU_RAISES2(etree::xml_error,
        [&]() {
            etree::XPath("&%^&%^&");
        },
        [&](etree::xml_error &e) {
            assert(e.what() == std::string("Invalid expression\n"));
        }
    );
}


MU_TEST(xpathCopyConstructor)
{
    auto xp = etree::XPath(".");
    auto xp2 = xp;
    assert(xp.expr() == xp2.expr());
}


MU_TEST(xpathExpr)
{
    auto xp = etree::XPath(".");
    assert(xp.expr() == ".");
}


MU_TEST(xpathFindOrder)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    assert(elem.child("a") == *xp.find(elem));
}


MU_TEST(xpathFindNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    assert(! xp.find(elem));
}


MU_TEST(xpathFindall)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    assert(elem.children() == xp.findall(elem));
}


MU_TEST(xpathFindallNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    assert(elem.children("nonexistent") == xp.findall(elem));
}


MU_TEST(xpathFindText)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("name");
    assert("David" == xp.findtext(elem));
}


MU_TEST(xpathFindTextDefault)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("age");
    assert("Unknown" == xp.findtext(elem, "Unknown"));
}


MU_MAIN()
