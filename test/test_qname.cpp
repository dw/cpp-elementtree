/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "myunit.hpp"


MU_TEST(ConstructNsTag)
{
    auto qn = etree::QName("ns", "tag");
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(ConstructCopy)
{
    auto qn = etree::QName("ns", "tag");
    auto qn2 = qn;
    assert(qn2.ns() == "ns");
    assert(qn2.tag() == "tag");
}


MU_TEST(ConstructUniversalName)
{
    auto qn = etree::QName(std::string("{ns}tag"));
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(ConstructUniversalNameChar)
{
    auto qn = etree::QName("{ns}tag");
    assert(qn.ns() == "ns");
    assert(qn.tag() == "tag");
}


MU_TEST(TostringNoNs)
{
    auto qn = etree::QName("nons");
    assert(qn.tostring() == "nons");
}


MU_TEST(TostringNs)
{
    auto qn = etree::QName("{urn:ns}nons");
    assert(qn.tostring() == "{urn:ns}nons");
}


MU_TEST(Equals)
{
    auto qn = etree::QName("{urn:ns}nons");
    assert(qn.equals("urn:ns", "nons"));
}


MU_TEST(EqualsFalse)
{
    auto qn = etree::QName("{urn:ns}nons");
    assert(! qn.equals("urn:ns", "ns"));
}


MU_TEST(EqualsFalseNoNs)
{
    auto qn = etree::QName("{urn:ns}nons");
    assert(! qn.equals(NULL, "nons"));
}


MU_TEST(EqualsFalseWrongTag)
{
    auto qn = etree::QName("{urn:ns}nons");
    assert(! qn.equals("urn:ns", "ns"));
}


MU_TEST(OpEqTrue)
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns}nons");
    assert(qn == qn2);
}


MU_TEST(OpFalseUnequalNs)
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns2}nons");
    assert(qn != qn2);
}


MU_TEST(OpEqFalseUnequalTag)
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns}ns");
    assert(qn != qn2);
}


MU_TEST(OpUnequalMissingNs)
{
    auto qn = etree::QName("nons");
    auto qn2 = etree::QName("{urn:ns2}nons");
    assert(qn != qn2);
}
