/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "catch.hpp"


TEST_CASE("DefaultConstructor", "[nullable]")
{
    auto val = etree::Nullable<etree::Element>();
    REQUIRE_FALSE(val);
}


TEST_CASE("Constructor", "[nullable]")
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    REQUIRE(val);
    REQUIRE(elem == *val);
}


TEST_CASE("RvalConstructor", "[nullable]")
{
    auto val = etree::Nullable<etree::Element>(etree::Element("a"));
    REQUIRE(val);
    REQUIRE(val->tag() == "a");
}


TEST_CASE("CopyConstructorUnset", "[nullable]")
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = val;
    REQUIRE_FALSE(val);
    REQUIRE_FALSE(val2);
    REQUIRE(val == val2);
}


TEST_CASE("CopyConstructorSet", "[nullable]")
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = val;
    REQUIRE(val);
    REQUIRE(val2);
    REQUIRE(val == val2);
}


TEST_CASE("AssignUnsetToUnset", "[nullable]")
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    REQUIRE_FALSE(val);
    REQUIRE_FALSE(val2);
}


TEST_CASE("AssignSetToUnset", "[nullable]")
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    REQUIRE(val);
    REQUIRE(val2);
    REQUIRE(*val == *val2);
}


TEST_CASE("AssignUnsetToSet", "[nullable]")
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>(elem);
    val2 = val;
    REQUIRE_FALSE(val);
    REQUIRE_FALSE(val2);
}


TEST_CASE("DerefUnset", "[nullable]")
{
    auto val = etree::Nullable<etree::Element>();
    REQUIRE_THROWS_AS(*val, etree::missing_value_error);
}


TEST_CASE("DerefSet", "[nullable]")
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    REQUIRE(val == elem);
}
