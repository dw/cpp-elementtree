
#include "element.hpp"
#include "gtest/gtest.h"

using etree::Element;


class ElementTest : public ::testing::Test
{
    protected:
    Element getRoot()
    {
        return etree::fromstring(
            "<who><person type='human'><name>David</name></person></who>"
        );
    }

    Element getNsRoot()
    {
        return etree::fromstring(
            "<foo:who xmlns:foo='urn:foo'>"
                "<foo:person foo:type='human'><name>David</name></foo:person>"
            "</foo:who>"
        );
    }
};


TEST_F(ElementTest, getNoNs) {
    Element root = getRoot();
    ASSERT_EQ("human", (*root.child("person")).get("type"));
}

TEST_F(ElementTest, getNs) {
    Element root = getNsRoot();
    #define NS "{urn:foo}"
    ASSERT_EQ("human", (*root.child(NS "person")).get(NS "type"));
}
