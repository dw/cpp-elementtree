/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <utility>
#include <vector>

#include "myunit.hpp"
#include "element.hpp"


MU_TEST(Constructor)
{
    auto xp = etree::XPath(".");
    assert(xp.expr() == ".");
}


MU_TEST(ConstructorParseError)
{
    auto e = myunit::raises<etree::xml_error>([&]() {
        etree::XPath("&%^&%^&");
    });
    assert(e.what() == std::string("Invalid expression\n"));
}


MU_TEST(CopyConstructor)
{
    auto xp = etree::XPath(".");
    auto xp2 = xp;
    assert(xp.expr() == xp2.expr());
}


MU_TEST(Expr)
{
    auto xp = etree::XPath(".");
    assert(xp.expr() == ".");
}


MU_TEST(FindOrder)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    assert(elem.child("a") == xp.find(elem));
}


MU_TEST(FindNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    assert(! xp.find(elem));
}


MU_TEST(Findall)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./*");
    assert(elem.children() == xp.findall(elem));
}


MU_TEST(FindallNoMatch)
{
    auto elem = etree::fromstring("<root><a/><b/><c/></root>");
    auto xp = etree::XPath("./nonexistent");
    assert(elem.children("nonexistent") == xp.findall(elem));
}


MU_TEST(FindText)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("name");
    assert("David" == xp.findtext(elem));
}


MU_TEST(FindTextDefault)
{
    auto elem = etree::fromstring("<root><name>David</name></root>");
    auto xp = etree::XPath("age");
    assert("Unknown" == xp.findtext(elem, "Unknown"));
}
