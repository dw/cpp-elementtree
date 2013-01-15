
#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>


namespace etree {


using std::string;
class AttrMap;
class DocProxy;
class Element;
class ElementTree;
class NodeProxy;
class QName;

Element SubElement(Element &parent, const QName &qname);
Element fromstring(const string &s);
Element XML(const string &s);
string tostring(const Element &e);
ElementTree parse(std::istream &is);
ElementTree parse(const string &path);
ElementTree parse(int fd);

std::ostream &operator<< (std::ostream &out, const ElementTree &elem);
std::ostream &operator<< (std::ostream &out, const Element &elem);
std::ostream &operator<< (std::ostream &out, const QName &qname);


template<typename T>
class Nullable {
    char val_[sizeof(T)];
    bool set_;

    public:
    Nullable();
    Nullable(const T &val);
    Nullable(const Nullable<T> &val);
    #if __cplusplus >= 201103L
    Nullable(T &&val);
    #endif
    ~Nullable();
    operator bool() const;
    T &operator *();
    const T &operator *() const;
};

typedef Nullable<Element> NElement;
typedef Nullable<string> NString;


class QName {
    string ns_;
    string tag_;

    void from_string(const string &qname);

    public:
    QName(const string &ns, const string &tag);
    QName(const QName &other);
    QName(const string &qname);
    QName(const char *qname);

    const string &tag() const;
    const string &ns() const;
    bool operator=(const QName &other);
};


class AttrIterator
{
    NodeProxy *proxy_;
    int state;

    public:
    AttrIterator(NodeProxy &proxy);
    QName key();
    string value();
    bool next();
};


class AttrMap
{
    NodeProxy &proxy_;

    public:
    ~AttrMap();
    AttrMap(NodeProxy &proxy);

    bool has(const QName &qname) const;
    string get(const QName &qname, const string &default_="") const;
    void set(const QName &qname, const string &s);
    std::vector<QName> keys() const;
};


class ElementTree
{
    DocProxy &proxy_;

    public:
    ~ElementTree();
    ElementTree();
    ElementTree(struct DocProxy &proxy);
};


class Element
{
    NodeProxy &proxy_;

    // Never defined.
    Element();
    void operator=(const Element&);

    public:
    ~Element();
    Element(const Element &e);
    Element(NodeProxy &node);

    static Element from_name(const QName &qname);
    Element(const QName &qname);

    #if __cplusplus >= 201103L
    Element(Element &&e);
    #endif

    size_t size() const;
    QName qname() const;
    string tag() const;
    string ns() const;
    AttrMap attrib() const;

    string get(const QName &qname, const string &default_="") const;

    Element operator[] (size_t i);

    bool isIndirectParent(const Element &e);
    void append(Element &e);
    void insert(size_t i, Element &e);
    void remove(Element &e);

    Nullable<Element> getnext() const;
    Nullable<Element> getparent() const;
    Nullable<Element> getprev() const;
    ElementTree getroottree() const;

    NodeProxy &proxy() const;
    string text() const;
    void text(const string &s);
    string tail() const;
    void tail(const string &s);
};


#define DEFINE_EXCEPTION(name)                          \
    struct name : public std::runtime_error {           \
        name() : std::runtime_error("etree::"#name) {}  \
    };

DEFINE_EXCEPTION(cyclical_tree_error)
DEFINE_EXCEPTION(element_error)
DEFINE_EXCEPTION(parse_error)
DEFINE_EXCEPTION(memory_error)
DEFINE_EXCEPTION(missing_namespace_error)
DEFINE_EXCEPTION(missing_value_error)
DEFINE_EXCEPTION(out_of_bounds_error)
DEFINE_EXCEPTION(qname_error)
DEFINE_EXCEPTION(serialization_error)

#undef DEFINE_EXCEPTION


} // namespace


#endif
