
#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"

#include "test_consts.hpp"


using etree::Element;


// ------------
// Constructors
// ------------


TEST_CASE("elemDestructor", "[element]")
{
    Element e("x");
}


TEST_CASE("elemKvList", "[element]")
{
    Element e("x", {
        {"a", "b"},
        {"c", "d"}
    });

    REQUIRE(e.attrib().get("a") == "b");
    REQUIRE_FALSE(e.attrib().has("b"));
    REQUIRE(e.attrib().get("c") == "d");
    REQUIRE_FALSE(e.attrib().has("d"));
}


// ---------
// Accessors
// ---------


TEST_CASE("elemQname", "[element]")
{
    Element e("x");
    REQUIRE(e.qname() == "x");
}


TEST_CASE("elemQnameNs", "[element]")
{
    Element e("{urn:woah}x");
    REQUIRE(e.qname() == "{urn:woah}x");
}


TEST_CASE("elemQnameSet", "[element]")
{
    Element e("x");
    e.qname("y");
    REQUIRE(e.qname() == "y");
}


TEST_CASE("elemQnameSetNs", "[element]")
{
    Element e("x");
    e.qname("{x}y");
    REQUIRE(e.qname() == "{x}y");
}


TEST_CASE("elemTag", "[element]")
{
    Element e("x");
    REQUIRE(e.tag() == "x");
}


TEST_CASE("elemTagSet", "[element]")
{
    Element e("x");
    e.tag("y");
    REQUIRE(e.tag() == "y");
    REQUIRE(e.ns() == "");
}


TEST_CASE("elemTagSetKeepNs", "[element]")
{
    Element e("{x}y");
    e.tag("z");
    REQUIRE(e.tag() == "z");
    REQUIRE(e.ns() == "x");
}


//
// ChildIterator
//


TEST_CASE("elemChildIter", "[element]")
{
    auto root = etree::fromstring(DOC);
    std::vector<std::string> qnames, expect {
        { "name" },
        { "{urn:ns}attr1" },
        { "{urn:ns}attr2" }
    };
    for(auto child : *root.child("person")) {
        qnames.push_back(child.qname().tostring());
    }
    REQUIRE(qnames == expect);
}


//
// visit()
//


TEST_CASE("visit", "[element]")
{
    auto root = etree::fromstring(DOC);
    std::vector<std::string> got, expect {
        { "who" },
        { "person" },
        { "name" },
        { "{urn:ns}attr1" },
        { "{urn:ns}attr2" }
    };
    visit(root, [&](Element &e) {
        got.push_back(e.qname().tostring());
    });
    REQUIRE(got == expect);
}


// ----------
// ancestorOf
// ----------


TEST_CASE("elemAncestorOfTrue", "[element]")
{
    auto root = etree::fromstring("<a><b/></a>");
    REQUIRE(root.ancestorOf(*root.child("b")));
}


TEST_CASE("elemAncestorOfFalse", "[element]")
{
    auto root = etree::fromstring("<a><b/></a>");
    REQUIRE_FALSE(root.child("b")->ancestorOf(root));
}


TEST_CASE("elemAncestorOfFalseWrongDoc", "[element]")
{
    auto root = etree::fromstring("<a><b/></a>");
    auto root2 = etree::fromstring("<a><b/></a>");
    REQUIRE_FALSE(root.ancestorOf(root2));
}


// ------
// append
// ------


TEST_CASE("elemAppendSelfFails", "[element]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE_THROWS_AS(root.append(root), etree::cyclical_tree_error);
}


TEST_CASE("elemAppendAncestorFails", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    REQUIRE_THROWS_AS(person.append(root), etree::cyclical_tree_error);
}


TEST_CASE("elemAppendNew", "[element]")
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    REQUIRE(root.size() == 1);
    REQUIRE(child == *root.child("child"));
}


TEST_CASE("elemAppendNewTwice", "[element]")
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.append(child);
    root.append(child);
    REQUIRE(root.size() == 1);
    REQUIRE(root.child("child") == child);
}


TEST_CASE("elemAppendDuplicateNs", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto child = etree::Element("{urn:ns}bar");
    child.attrib().set("{urn:ns}baz", "1");
    root.append(child);
    REQUIRE(etree::tostring(child) == "<ns:bar ns:baz=\"1\"/>");
}


TEST_CASE("elemAppendMoveNsSimple1", "[element]")
{
    auto root = etree::fromstring("<a xmlns:ns=\"urn:ns\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.append(root2);
    REQUIRE(etree::tostring(root) == "<a xmlns:ns=\"urn:ns\"><ns:b/></a>");
}


TEST_CASE("elemAppendMoveNsSimple2", "[element]")
{
    auto root = etree::fromstring("<a xmlns=\"urn:ns\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.append(root2);
    REQUIRE(etree::tostring(root) == "<a xmlns=\"urn:ns\"><b/></a>");
}


TEST_CASE("elemAppendMoveNsNested", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto root2 = etree::fromstring(DOC);
    root.append(*root2.find("person/name"));
    // REQUIRE(0)
}


//
// Element::child()
//


TEST_CASE("child", "[element]")
{
    auto root = etree::fromstring("<root><child>x</child></root>");
    auto child = root.child("child");
    REQUIRE(child);
    REQUIRE(child->text() == "x");
}


TEST_CASE("childNs", "[element]")
{
    auto root = etree::fromstring(
        "<root>"
        "<child xmlns=\"urn:foo\">x</child>"
        "</root>");
    auto child = root.child("{urn:foo}child");
    REQUIRE(child);
    REQUIRE(child->text() == "x");
}


TEST_CASE("childAbsent", "[element]")
{
    auto root = etree::fromstring("<root/>");
    auto child = root.child("{urn:foo}child");
    REQUIRE_FALSE(child);
}


//
// Element::ensurechild()
//


TEST_CASE("ensurechildPresent", "[element]")
{
    auto root = etree::fromstring("<root><child>x</child></root>");
    auto child = root.ensurechild("child");
    REQUIRE(child.text() == "x");
    REQUIRE(root.children("child").size() == 1);
}


TEST_CASE("ensurechildNsPresent", "[element]")
{
    auto root = etree::fromstring(
        "<root>"
        "<child xmlns=\"urn:foo\">x</child>"
        "</root>");
    auto child = root.ensurechild("{urn:foo}child");
    REQUIRE(child.text() == "x");
    REQUIRE(root.children("{urn:foo}child").size() == 1);
}


TEST_CASE("ensurechildAbsent", "[element]")
{
    auto root = etree::fromstring("<root/>");
    auto child = root.ensurechild("child");
    REQUIRE(child.getparent() == root);
    REQUIRE(root.children("child").size() == 1);
}


TEST_CASE("ensurechildAbsentNs", "[element]")
{
    auto root = etree::fromstring("<root/>");
    auto child = root.ensurechild("{urn:foo}child");
    REQUIRE(child.getparent() == root);
    REQUIRE(root.children("{urn:foo}child").size() == 1);
}


//
// Element::ensurens()
//

TEST_CASE("ensurens", "[element]")
{
    auto root = etree::fromstring("<root/>");
    root.ensurens("urn:foo");
    REQUIRE(etree::tostring(root) == "<root xmlns:ns0=\"urn:foo\"/>");
}


TEST_CASE("ensurensExisting", "[element]")
{
    auto root = etree::fromstring("<root xmlns:ns0=\"urn:foo\"/>");
    root.ensurens("urn:foo");
    REQUIRE(etree::tostring(root) == "<root xmlns:ns0=\"urn:foo\"/>");
}


TEST_CASE("ensurensOnParent", "[element]")
{
    auto root = etree::fromstring(
        "<root xmlns:ns0=\"urn:foo\">"
            "<child/>"
        "</root>");
    root.child("child")->ensurens("urn:foo");
    REQUIRE(etree::tostring(root) == (
        "<root xmlns:ns0=\"urn:foo\">"
            "<child/>"
        "</root>"
    ));
}


//
// getnext / getparent / getprev / getroottreee
//


TEST_CASE("elemGetnextNone", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE_FALSE(root.child("c")->getnext());
}


TEST_CASE("elemGetnext", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(root.child("b") == root.child("a")->getnext());
}


TEST_CASE("elemGetprevNone", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE_FALSE(root.child("a")->getprev());
}


TEST_CASE("elemGetParentRoot", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE_FALSE(root.getparent());
}


TEST_CASE("elemGetParentNotroot", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(root.child("a")->getparent() == root);
}


TEST_CASE("elemGetroottree", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(root.getroottree() == root.getroottree());
}


TEST_CASE("elemGetroottreeDifferentDocs", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    auto root2 = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(root.getroottree() != root2.getroottree());
}


TEST_CASE("elemGetroottreeRemoved", "[element]")
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    auto elem = *root.child("a");
    elem.remove();
    REQUIRE(root.getroottree() != elem.getroottree());
}


// ------
// insert
// -----


TEST_CASE("elemInsertSelfFails", "[element]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE_THROWS_AS(root.insert(0, root), etree::cyclical_tree_error);
}


TEST_CASE("elemInsertAncestorFails", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    REQUIRE_THROWS_AS(person.insert(0, root), etree::cyclical_tree_error);
}


TEST_CASE("elemInsertNew", "[element]")
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.insert(0, child);
    REQUIRE(root.size() == 1);
    REQUIRE(root.child("child") == child);
}


TEST_CASE("elemInsertNewTwice", "[element]")
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.insert(0, child);
    root.insert(0, child);
    REQUIRE(root.size() == 1);
    REQUIRE(root.child("child") == child);
}


TEST_CASE("elemInsertDuplicateNs", "[element]")
{
    auto root = etree::fromstring(
        "<who xmlns:ns=\"urn:ns\">"
            "<ns:person />"
        "</who>"
    );
    auto child = etree::Element("{urn:ns}child", {
        {"{urn:ns}attr", "1"},
    });
    root.insert(0, child);
    REQUIRE(etree::tostring(root) == (
        "<who xmlns:ns=\"urn:ns\">"
            "<ns:child ns:attr=\"1\"/>"
            "<ns:person/>"
        "</who>"
    ));
}


TEST_CASE("elemInsertIndexZeroWhileEmpty", "[element]")
{
    auto root = etree::Element("a");
    auto child = etree::Element("b");
    root.insert(0, child);
    REQUIRE(etree::tostring(root) == "<a><b/></a>");
}


TEST_CASE("elemInsertIndexPastEnd", "[element]")
{
    auto root = etree::fromstring("<a><b/></a>");
    auto child = etree::Element("c");
    root.insert(100, child);
    REQUIRE(etree::tostring(root) == "<a><b/><c/></a>");
}


TEST_CASE("elemInsertMoveNsSimple1", "[element]")
{
    auto root = etree::fromstring("<a xmlns:ns=\"urn:ns\"><c/></a>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.insert(0, root2);
    REQUIRE(etree::tostring(root) == "<a xmlns:ns=\"urn:ns\"><ns:b/><c/></a>");
}


TEST_CASE("elemInsertMoveNsSimple2", "[element]")
{
    auto root = etree::fromstring("<a xmlns=\"urn:ns\"><c/></a>");
    auto root2 = etree::fromstring("<foo:b xmlns:foo=\"urn:ns\"/>");
    root.insert(0, root2);
    REQUIRE(etree::tostring(root) == "<a xmlns=\"urn:ns\"><b/><c/></a>");
}


// ------
// remove
// ------


TEST_CASE("elemRemoveNoArg", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    REQUIRE_FALSE(person.getparent());
    REQUIRE_FALSE(root.child("person"));
}


TEST_CASE("elemRemoveArg", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    REQUIRE_FALSE(person.getparent());
    REQUIRE_FALSE(root.child("person"));
}


TEST_CASE("elemRemoveArgNotParent", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    root.remove(name);
    REQUIRE(root.size() == 1);
    REQUIRE(name.getparent()->tag() == "person");
    REQUIRE_FALSE(root.child("name"));
}


TEST_CASE("elemRemoveTwiceNoArgs", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    person.remove();
    person.remove();
    REQUIRE_FALSE(root.child("person"));
}


TEST_CASE("elemRemoveSucceeds", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
}


TEST_CASE("elemRemoveTwiceOkay", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.remove(person);
}


TEST_CASE("elemRemoveThenAppend", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    root.remove(person);
    root.append(person);
    REQUIRE(DOC == etree::tostring(root));
}


TEST_CASE("elemRemoveNsPreserved", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    REQUIRE(etree::tostring(name) ==
           "<name xmlns:ns0=\"urn:ns\" ns0:attrx=\"3\">David</name>");
}


TEST_CASE("elemRemoveAddNsCollapsed", "[element]")
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    root.append(name);
    REQUIRE(etree::tostring(name) ==
           "<name ns:attrx=\"3\">David</name>");
}


TEST_CASE("elemRemovePreservesTail", "[element]")
{
    auto elem = etree::Element("person");
    auto e2 = etree::SubElement(elem, "name");
    e2.tail("\n\n");
    e2.remove();
    elem.append(e2);
    REQUIRE(etree::tostring(elem) == (
        "<person><name/>\n"
        "\n"
        "</person>"
    ));
}


TEST_CASE("elemRemovePreservesTailTextOnly", "[element]")
{
    auto elem = etree::fromstring("<a><b/><c/></a>");
    elem.child("b")->remove();
    REQUIRE(etree::tostring(elem) == "<a><c/></a>");
}


TEST_CASE("elemText", "[element]")
{
    auto elem = etree::fromstring("<name>David</name>");
    REQUIRE(elem.text() == "David");
}


TEST_CASE("elemTextSet", "[element]")
{
    auto elem = etree::fromstring("<name/>");
    elem.text("David");
    REQUIRE("<name>David</name>" == etree::tostring(elem));
}


TEST_CASE("elemTextSetEmpty", "[element]")
{
    auto elem = etree::fromstring("<name>David</name>");
    elem.text("");
    REQUIRE("<name/>" == etree::tostring(elem));
}


TEST_CASE("elemTextSetChildElements", "[element]")
{
    auto elem = etree::fromstring("<name><lang/></name>");
    elem.text("David");
    REQUIRE("<name>David<lang/></name>" == etree::tostring(elem));
}


//
// tostring
//


TEST_CASE("elemTostring", "[element]")
{
    auto elem = etree::Element("name");
    elem.text("David");
    elem.attrib().set({
        {"{urn:ns}x", "1"},
        {"{urn:bar}y", "2"}
    });

    auto got = etree::tostring(elem);
    auto expect = ("<name xmlns:ns0=\"urn:ns\" xmlns:ns1=\"urn:bar\" "
                  "ns0:x=\"1\" ns1:y=\"2\">David</name>");
    REQUIRE(got == expect);
}


TEST_CASE("treeTostring", "[element]")
{
    auto elem = etree::Element("name");
    elem.text("David");
    elem.attrib().set({
        {"{urn:ns}x", "1"},
        {"{urn:bar}y", "2"}
    });

    auto got = etree::tostring(elem.getroottree());
    auto expect = ("<?xml version=\"1.0\"?>\n"
                   "<name xmlns:ns0=\"urn:ns\" xmlns:ns1=\"urn:bar\" "
                    "ns0:x=\"1\" ns1:y=\"2\">David</name>\n");
    REQUIRE(got == expect);
}


// ---
// Rest??
// ---


TEST_CASE("elemGetNoNs", "[element]")
{
    auto root = etree::fromstring(DOC);
    REQUIRE("human" == root.child("person")->get("type"));
}


TEST_CASE("elemGetNs", "[element]")
{
    auto root = etree::fromstring(NS_DOC);
    #define NS "{urn:ns}"
    REQUIRE("human" == root.child(NS "person")->get(NS "type"));
}


//
// Element::find()
//


TEST_CASE("elemFindOrder", "[element]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(elem.child("a") == *elem.find("./*"));
}


TEST_CASE("elemFindNoMatch", "[element]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE_FALSE(elem.find("./nonexistent"));
}


//
// Element::copy()
//


TEST_CASE("copy", "[element]")
{
    auto e = etree::fromstring("<root><a/><b/><c/></root>");
    auto e2 = e.copy();
    REQUIRE(e != e2);
    REQUIRE(e.getroottree() != e2.getroottree());
    e.attrib().set("test", "test");
    REQUIRE(e2.attrib().get("test") == "");
}


TEST_CASE("copyNs", "[element]")
{
    auto e = etree::fromstring("<root xmlns:foo=\"urn:foo\"><foo:a/></root>");
    auto e2 = e.child("{urn:foo}a")->copy();
    REQUIRE(etree::tostring(e2) == "<foo:a xmlns:foo=\"urn:foo\"/>");
}


//
// Element::findall()
//


TEST_CASE("elemFindall", "[element]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(elem.children() == elem.findall("./*"));
}


TEST_CASE("elemFindallNoMatch", "[element]")
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    REQUIRE(elem.children("missing") == elem.findall("./missing"));
}


//
// Element::findtext()
//


TEST_CASE("elemFindtext", "[element]")
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    REQUIRE("David" == elem.findtext("name"));
}


TEST_CASE("elemFindtextDefault", "[element]")
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    REQUIRE("Unknown" == elem.findtext("age", "Unknown"));
}


//
// Element::graft()
//


TEST_CASE("graft", "[element]")
{
    auto elem = etree::fromstring(
        "<root>"
            "<tag1/> Hello"
            "<tag2>"
                "<tag3/>"
            "</tag2> there"
        "</root>"
    );
    elem.child("tag2")->graft();
    REQUIRE(etree::tostring(elem) == (
        "<root>"
            "<tag1/> Hello"
            "<tag3/> there"
        "</root>"
    ));
}


//
// Element::operator[]
//


TEST_CASE("elemIndexInBounds", "[element]")
{
    auto elem = etree::fromstring("<root><child/></root>");
    REQUIRE(*elem.child("child") == elem[0]);
}


TEST_CASE("elemIndexOutOfBounds", "[element]")
{
    auto elem = etree::fromstring("<root><child/></root>");
    REQUIRE_THROWS_AS(elem[1], etree::out_of_bounds_error);
}


TEST_CASE("elemIndexOutOfBoundsNoChildren", "[element]")
{
    auto elem = etree::fromstring("<root/>");
    REQUIRE_THROWS_AS(elem[0], etree::out_of_bounds_error);
}



TEST_CASE("elemIndexNegative", "[element]")
{
    auto elem = etree::fromstring("<root><child/></root>");
    REQUIRE_THROWS_AS(elem[-1], etree::out_of_bounds_error);
}
