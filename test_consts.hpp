
static auto DOC = (
    "<who xmlns:ns=\"urn:ns\" type=\"people\" count=\"1\" ns:x=\"true\">"
        "<person type=\"human\">"
            "<name ns:attrx=\"3\">David</name>"
            "<ns:attr1>123</ns:attr1>"
            "<ns:attr2>123</ns:attr2>"
        "</person>"
    "</who>"
);

static auto NS_DOC = (
    "<ns:who xmlns:ns='urn:ns'>"
        "<ns:person ns:type='human'><name>David</name></ns:person>"
    "</ns:who>"
);

#define OUT(x) std::cout << x << std::endl;
#define TOSTRING(x) OUT(etree::tostring(x))
