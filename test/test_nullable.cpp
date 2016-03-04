/*
 * Copyright David Wilson, 2016.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <utility>
#include <vector>

#include <elementtree.hpp>

#include "myunit.hpp"


MU_TEST(DefaultConstructor)
{
    auto val = etree::Nullable<etree::Element>();
    assert(! val);
}


MU_TEST(Constructor)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    assert(val);
    assert(elem == *val);
}


MU_TEST(RvalConstructor)
{
    auto val = etree::Nullable<etree::Element>(etree::Element("a"));
    assert(val);
    assert(val->tag() == "a");
}


MU_TEST(CopyConstructorUnset)
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = val;
    assert(! val);
    assert(! val2);
    assert(val == val2);
}


MU_TEST(CopyConstructorSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = val;
    assert(val);
    assert(val2);
    assert(val == val2);
}


MU_TEST(AssignUnsetToUnset)
{
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    assert(! val);
    assert(! val2);
}


MU_TEST(AssignSetToUnset)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    auto val2 = etree::Nullable<etree::Element>();
    val2 = val;
    assert(val);
    assert(val2);
    assert(*val == *val2);
}


MU_TEST(AssignUnsetToSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>();
    auto val2 = etree::Nullable<etree::Element>(elem);
    val2 = val;
    assert(! val);
    assert(! val2);
}


MU_TEST(DerefUnset)
{
    auto val = etree::Nullable<etree::Element>();
    myunit::raises<etree::missing_value_error>([&]() {
        *val;
    });
}


MU_TEST(DerefSet)
{
    auto elem = etree::Element("a");
    auto val = etree::Nullable<etree::Element>(elem);
    assert(val == elem);
}
