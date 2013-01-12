
#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

#include <iostream>
#include <cstdio>
#include <vector>


namespace etree {


class AttrMap;
class DocProxy;
class Element;
class ElementTree;
class NodeProxy;
class QName;

Element SubElement(Element &parent, const QName &uname);
Element fromstring(const std::string &s);
std::string tostring(const Element &e);
ElementTree parse(FILE *fp);
ElementTree parse(const std::istream &is);
ElementTree parse(const std::string &path);
ElementTree parse(int fd);

std::ostream &operator<< (std::ostream &out, const Element &elem);
std::ostream &operator<< (std::ostream &out, const QName &un);


class QName {
    std::string ns_;
    std::string tag_;

    void from_string(const std::string &uname);

    public:
    QName(const std::string &ns, const std::string &tag);
    QName(const QName &other);
    QName(const std::string &uname);
    QName(const char *uname);

    const std::string &tag() const;
    const std::string &ns() const;
    bool operator=(const QName &other);
};


class AttrMap
{
    NodeProxy *proxy_;

    public:
    ~AttrMap();
    AttrMap(NodeProxy *proxy);

    bool has(const QName &un) const;
    std::string get(const QName &un, const std::string &default_="") const;
    void set(const QName &un, const std::string &s);
    std::vector<QName> keys() const;
};


class ElementTree
{
    DocProxy *proxy_;

    public:
    ~ElementTree();
    ElementTree();
};


class Element
{
    NodeProxy *proxy_;

    public:
    ~Element();
    Element();
    Element(const Element &e);
    Element(NodeProxy *node);
    Element(const QName &un);

    #if __cplusplus >= 201103L
    Element(Element &&e);
    #endif

    size_t size() const;
    QName uname() const;
    const char *tag() const;
    const char *ns() const;
    AttrMap attrib() const;
    std::string get(const QName &un, const std::string &default_="") const;
    operator bool() const;
    Element operator[] (size_t i);
    bool isIndirectParent(const Element &e);
    void append(Element &e);
    void insert(size_t i, Element &e);
    void remove(Element &e);
    Element getparent() const;

    NodeProxy *proxy() const;
    std::string text() const;
    void text(const std::string &s);
    std::string tail() const;
    void tail(const std::string &s);
};


#define DEFINE_EXCEPTION(name)                          \
    struct name : public std::runtime_error {           \
        name() : std::runtime_error("etree::"#name) {}  \
    };

DEFINE_EXCEPTION(cyclical_tree_error)
DEFINE_EXCEPTION(element_error)
DEFINE_EXCEPTION(empty_element_error)
DEFINE_EXCEPTION(memory_error)
DEFINE_EXCEPTION(missing_namespace_error)
DEFINE_EXCEPTION(out_of_bounds_error)
DEFINE_EXCEPTION(serialization_error)
DEFINE_EXCEPTION(qname_error)

#undef DEFINE_EXCEPTION


} // namespace


#endif
