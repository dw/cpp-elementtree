
#include <cassert>
#include <utility>
#include <vector>

#include "myunit.hpp"

#include "element.hpp"
#include "test_consts.hpp"


using etree::Element;


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


//
// ChildIterator
//

MU_TEST(elemChildIter)
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
    assert(qnames == expect);
}


//
// visit()
//


MU_TEST(visit)
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
    assert(got == expect);
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
    assert(! root.child("b")->ancestorOf(root));
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
    myunit::raises<etree::cyclical_tree_error>([&]() {
        root.append(root);
    });
}


MU_TEST(elemAppendAncestorFails)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    myunit::raises<etree::cyclical_tree_error>([&]() {
        person.append(root);
    });
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
    assert(root.child("child") == child);
}


MU_TEST(elemAppendDuplicateNs)
{
    auto root = etree::fromstring(DOC);
    auto child = etree::Element("{urn:ns}bar");
    child.attrib().set("{urn:ns}baz", "1");
    root.append(child);
    assert(etree::tostring(child) == "<ns:bar ns:baz=\"1\"/>");
}


MU_TEST(elemAppendMoveNsSimple1)
{
    auto root = etree::fromstring("<a xmlns:ns=\"urn:ns\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.append(root2);
    assert(etree::tostring(root) == "<a xmlns:ns=\"urn:ns\"><ns:b/></a>");
}


MU_TEST(elemAppendMoveNsSimple2)
{
    auto root = etree::fromstring("<a xmlns=\"urn:ns\"/>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.append(root2);
    assert(etree::tostring(root) == "<a xmlns=\"urn:ns\"><b/></a>");
}


MU_TEST(elemAppendMoveNsNested)
{
    auto root = etree::fromstring(DOC);
    auto root2 = etree::fromstring(DOC);
    root.append(*root2.find("person/name"));
    // ASsert(0)
}


//
// Element::child()
//


MU_TEST(child)
{
    auto root = etree::fromstring("<root><child>x</child></root>");
    auto child = root.child("child");
    assert(child);
    assert(child->text() == "x");
}


MU_TEST(childNs)
{
    auto root = etree::fromstring(
        "<root>"
        "<child xmlns=\"urn:foo\">x</child>"
        "</root>");
    auto child = root.child("{urn:foo}child");
    assert(child);
    assert(child->text() == "x");
}


MU_TEST(childAbsent)
{
    auto root = etree::fromstring("<root/>");
    auto child = root.child("{urn:foo}child");
    assert(! child);
}


//
// Element::ensurechild()
//


MU_TEST(ensurechildPresent)
{
    auto root = etree::fromstring("<root><child>x</child></root>");
    auto child = root.ensurechild("child");
    assert(child.text() == "x");
    assert(root.children("child").size() == 1);
}


MU_TEST(ensurechildNsPresent)
{
    auto root = etree::fromstring(
        "<root>"
        "<child xmlns=\"urn:foo\">x</child>"
        "</root>");
    auto child = root.ensurechild("{urn:foo}child");
    assert(child.text() == "x");
    assert(root.children("{urn:foo}child").size() == 1);
}


MU_TEST(ensurechildAbsent)
{
    auto root = etree::fromstring("<root/>");
    auto child = root.ensurechild("child");
    assert(child.getparent() == root);
    assert(root.children("child").size() == 1);
}


MU_TEST(ensurechildAbsentNs)
{
    auto root = etree::fromstring("<root/>");
    auto child = root.ensurechild("{urn:foo}child");
    assert(child.getparent() == root);
    assert(root.children("{urn:foo}child").size() == 1);
}


//
// Element::ensurens()
//

MU_TEST(ensurens)
{
    auto root = etree::fromstring("<root/>");
    root.ensurens("urn:foo");
    assert(etree::tostring(root) == "<root xmlns:ns0=\"urn:foo\"/>");
}


MU_TEST(ensurensExisting)
{
    auto root = etree::fromstring("<root xmlns:ns0=\"urn:foo\"/>");
    root.ensurens("urn:foo");
    assert(etree::tostring(root) == "<root xmlns:ns0=\"urn:foo\"/>");
}


MU_TEST(ensurensOnParent)
{
    auto root = etree::fromstring(
        "<root xmlns:ns0=\"urn:foo\">"
            "<child/>"
        "</root>");
    root.child("child")->ensurens("urn:foo");
    assert(etree::tostring(root) == (
        "<root xmlns:ns0=\"urn:foo\">"
            "<child/>"
        "</root>"
    ));
}


//
// getnext / getparent / getprev / getroottreee
//


MU_TEST(elemGetnextNone)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(! root.child("c")->getnext());
}


MU_TEST(elemGetnext)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(root.child("b") == root.child("a")->getnext());
}


MU_TEST(elemGetprevNone)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(! root.child("a")->getprev());
}


MU_TEST(elemGetParentRoot)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(! root.getparent());
}


MU_TEST(elemGetParentNotroot)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(root.child("a")->getparent() == root);
}


MU_TEST(elemGetroottree)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    assert(root.getroottree() == root.getroottree());
}


MU_TEST(elemGetroottreeDifferentDocs)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    auto root2 = etree::fromstring("<root><a/><b/><c/></root>");
    assert(root.getroottree() != root2.getroottree());
}


MU_TEST(elemGetroottreeRemoved)
{
    auto root = etree::fromstring("<root><a/><b/><c/></root>");
    auto elem = *root.child("a");
    elem.remove();
    assert(root.getroottree() != elem.getroottree());
}


// ------
// insert
// -----


MU_TEST(elemInsertSelfFails)
{
    auto root = etree::fromstring(DOC);
    myunit::raises<etree::cyclical_tree_error>([&]() {
        root.insert(0, root);
    });
}


MU_TEST(elemInsertAncestorFails)
{
    auto root = etree::fromstring(DOC);
    auto person = *root.child("person");
    myunit::raises<etree::cyclical_tree_error>([&]() {
        person.insert(0, root);
    });
}


MU_TEST(elemInsertNew)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.insert(0, child);
    assert(root.size() == 1);
    assert(root.child("child") == child);
}


MU_TEST(elemInsertNewTwice)
{
    auto root = etree::Element("root");
    auto child = etree::Element("child");
    root.insert(0, child);
    root.insert(0, child);
    assert(root.size() == 1);
    assert(root.child("child") == child);
}


MU_TEST(elemInsertDuplicateNs)
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
    assert(etree::tostring(root) == (
        "<who xmlns:ns=\"urn:ns\">"
            "<ns:child ns:attr=\"1\"/>"
            "<ns:person/>"
        "</who>"
    ));
}


MU_TEST(elemInsertIndexZeroWhileEmpty)
{
    auto root = etree::Element("a");
    auto child = etree::Element("b");
    root.insert(0, child);
    assert(etree::tostring(root) == "<a><b/></a>");
}


MU_TEST(elemInsertIndexPastEnd)
{
    auto root = etree::fromstring("<a><b/></a>");
    auto child = etree::Element("c");
    root.insert(100, child);
    assert(etree::tostring(root) == "<a><b/><c/></a>");
}


MU_TEST(elemInsertMoveNsSimple1)
{
    auto root = etree::fromstring("<a xmlns:ns=\"urn:ns\"><c/></a>");
    auto root2 = etree::fromstring("<b xmlns=\"urn:ns\"/>");
    root.insert(0, root2);
    assert(etree::tostring(root) == "<a xmlns:ns=\"urn:ns\"><ns:b/><c/></a>");
}


MU_TEST(elemInsertMoveNsSimple2)
{
    auto root = etree::fromstring("<a xmlns=\"urn:ns\"><c/></a>");
    auto root2 = etree::fromstring("<foo:b xmlns:foo=\"urn:ns\"/>");
    root.insert(0, root2);
    assert(etree::tostring(root) == "<a xmlns=\"urn:ns\"><b/><c/></a>");
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
    assert(name.getparent()->tag() == "person");
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
           "<name xmlns:ns0=\"urn:ns\" ns0:attrx=\"3\">David</name>");
}


MU_TEST(elemRemoveAddNsCollapsed)
{
    auto root = etree::fromstring(DOC);
    auto name = *root.find("person/name");
    name.remove();
    root.append(name);
    assert(etree::tostring(name) ==
           "<name ns:attrx=\"3\">David</name>");
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
    elem.child("b")->remove();
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
// tostring
//


MU_TEST(elemTostring)
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
    assert(got == expect);
}


MU_TEST(treeTostring)
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
    assert(got == expect);
}


// ---
// Rest??
// ---


MU_TEST(elemGetNoNs)
{
    auto root = etree::fromstring(DOC);
    assert("human" == root.child("person")->get("type"));
}


MU_TEST(elemGetNs)
{
    auto root = etree::fromstring(NS_DOC);
    #define NS "{urn:ns}"
    assert("human" == root.child(NS "person")->get(NS "type"));
}


//
// Element::find()
//


MU_TEST(elemFindOrder)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    assert(elem.child("a") == *elem.find("./*"));
}


MU_TEST(elemFindNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    assert(! elem.find("./nonexistent"));
}


//
// Element::copy()
//


MU_TEST(copy)
{
    auto e = etree::fromstring("<root><a/><b/><c/></root>");
    auto e2 = e.copy();
    assert(e != e2);
    assert(e.getroottree() != e2.getroottree());
    e.attrib().set("test", "test");
    assert(e2.attrib().get("test") == "");
}


MU_TEST(copyNs)
{
    auto e = etree::fromstring("<root xmlns:foo=\"urn:foo\"><foo:a/></root>");
    auto e2 = e.child("{urn:foo}a")->copy();
    assert(etree::tostring(e2) == "<foo:a xmlns:foo=\"urn:foo\"/>");
}


//
// Element::findall()
//


MU_TEST(elemFindall)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    assert(elem.children() == elem.findall("./*"));
}


MU_TEST(elemFindallNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    assert(elem.children("missing") == elem.findall("./missing"));
}


//
// Element::findtext()
//


MU_TEST(elemFindtext)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    assert("David" == elem.findtext("name"));
}


MU_TEST(elemFindtextDefault)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    assert("Unknown" == elem.findtext("age", "Unknown"));
}


//
// Element::graft()
//


MU_TEST(graft)
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
    assert(etree::tostring(elem) == (
        "<root>"
            "<tag1/> Hello"
            "<tag3/> there"
        "</root>"
    ));
}


//
// Element::operator[]
//

MU_TEST(elemIndexInBounds)
{
    auto elem = etree::fromstring("<root><child/></root>");
    assert(*elem.child("child") == elem[0]);
}


MU_TEST(elemIndexOutOfBounds)
{
    auto elem = etree::fromstring("<root><child/></root>");
    myunit::raises<etree::out_of_bounds_error>([&]() {
        elem[1];
    });
}


MU_TEST(elemIndexOutOfBoundsNoChildren)
{
    auto elem = etree::fromstring("<root/>");
    myunit::raises<etree::out_of_bounds_error>([&]() {
        elem[0];
    });
}



MU_TEST(elemIndexNegative)
{
    auto elem = etree::fromstring("<root><child/></root>");
    auto e = myunit::raises<etree::out_of_bounds_error>([&]() {
        elem[-1];
    });
}
