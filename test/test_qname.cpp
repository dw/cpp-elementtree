/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"


TEST_CASE("ConstructNsTag", "[qname]")
{
    auto qn = etree::QName("ns", "tag");
    REQUIRE(qn.ns() == "ns");
    REQUIRE(qn.tag() == "tag");
}


TEST_CASE("ConstructCopy", "[qname]")
{
    auto qn = etree::QName("ns", "tag");
    auto qn2 = qn;
    REQUIRE(qn2.ns() == "ns");
    REQUIRE(qn2.tag() == "tag");
}


TEST_CASE("ConstructUniversalName", "[qname]")
{
    auto qn = etree::QName(std::string("{ns}tag"));
    REQUIRE(qn.ns() == "ns");
    REQUIRE(qn.tag() == "tag");
}


TEST_CASE("ConstructUniversalNameChar", "[qname]")
{
    auto qn = etree::QName("{ns}tag");
    REQUIRE(qn.ns() == "ns");
    REQUIRE(qn.tag() == "tag");
}


TEST_CASE("TostringNoNs", "[qname]")
{
    auto qn = etree::QName("nons");
    REQUIRE(qn.tostring() == "nons");
}


TEST_CASE("TostringNs", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    REQUIRE(qn.tostring() == "{urn:ns}nons");
}


TEST_CASE("Equals", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    REQUIRE(qn.equals("urn:ns", "nons"));
}


TEST_CASE("EqualsFalse", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    REQUIRE(! qn.equals("urn:ns", "ns"));
}


TEST_CASE("EqualsFalseNoNs", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    REQUIRE_FALSE(qn.equals(NULL, "nons"));
}


TEST_CASE("EqualsFalseWrongTag", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    REQUIRE_FALSE(qn.equals("urn:ns", "ns"));
}


TEST_CASE("OpEqTrue", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns}nons");
    REQUIRE(qn == qn2);
}


TEST_CASE("OpFalseUnequalNs", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns2}nons");
    REQUIRE(qn != qn2);
}


TEST_CASE("OpEqFalseUnequalTag", "[qname]")
{
    auto qn = etree::QName("{urn:ns}nons");
    auto qn2 = etree::QName("{urn:ns}ns");
    REQUIRE(qn != qn2);
}


TEST_CASE("OpUnequalMissingNs", "[qname]")
{
    auto qn = etree::QName("nons");
    auto qn2 = etree::QName("{urn:ns2}nons");
    REQUIRE(qn != qn2);
}
