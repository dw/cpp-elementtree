
#include "element.hpp"


namespace etree {


Element SubElement(Element &parent, const UniversalName &uname)
{
    if(! parent) {
        throw element_error("cannot create sub-element on empty element.");
    }

    Element elem(uname);
    parent.append(elem);
    return elem;
}


Element fromstring(const std::string &s)
{
    xmlDocPtr doc = ::xmlReadMemory(s.data(), s.size(), 0, 0, 0);
    if(! doc) {
        return Element();
    }

    return Element(doc->children);
}


static int writeCallback(void *ctx, const char *buffer, int len)
{
    std::string *s = static_cast<std::string *>(ctx);
    s->append(buffer, len);
    return len;
}


static int closeCallback(void *ctx)
{
    return 0;
}


std::string tostring(const Element &e)
{
    std::string out;
    xmlSaveCtxtPtr ctx = ::xmlSaveToIO(writeCallback, closeCallback,
        static_cast<void *>(&out), 0, 0);

    int ret = ::xmlSaveTree(ctx, e._node());
    ::xmlSaveClose(ctx);
    if(ret == -1) {
        throw serialization_error();
    }

    return out;
}


std::ostream &operator<< (std::ostream &out, const Element &elem)
{
    if(elem) {
        out << "<Element " << elem.tag() << " with " << elem.size();
        out << " children>";
    } else {
        out << "<empty Element>";
    }
    return out;
}

}
