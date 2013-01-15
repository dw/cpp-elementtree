
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

#include <libxml/tree.h>


namespace etree {


using std::string;
class AttrMap;
class Element;
class ElementTree;
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
    xmlNodePtr node_;

    public:
    AttrIterator(xmlNodePtr elem);
    QName key();
    string value();
    bool next();
};


class AttrMap
{
    xmlNodePtr node_;

    public:
    ~AttrMap();
    AttrMap(xmlNodePtr elem);

    bool has(const QName &qname) const;
    string get(const QName &qname, const string &default_="") const;
    void set(const QName &qname, const string &s);
    std::vector<QName> keys() const;
};


class ElementTree
{
    xmlDocPtr doc_;

    public:
    ~ElementTree();
    ElementTree();
    ElementTree(xmlDocPtr doc);
};


class Element
{
    xmlNodePtr node_;

    // Never defined.
    Element();
    void operator=(const Element&);

    public:
    ~Element();
    Element(const Element &e);
    Element(xmlNodePtr node);

    static Element from_name(const QName &qname);
    Element(const QName &qname);

    #if __cplusplus >= 201103L
    Element(Element &&e);
    #endif

    size_t size() const;
    QName qname() const;

    string tag() const;
    void tag(const string &tag);

    string ns() const;
    void ns(const string &ns);

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

    xmlNodePtr _node() const;
    string text() const;
    void text(const string &s);
    string tail() const;
    void tail(const string &s);
};


#define EXCEPTION(name)                                 \
    struct name : public std::runtime_error {           \
        name() : std::runtime_error("etree::"#name) {}  \
    };

EXCEPTION(cyclical_tree_error)
EXCEPTION(element_error)
EXCEPTION(parse_error)
EXCEPTION(memory_error)
EXCEPTION(missing_namespace_error)
EXCEPTION(missing_value_error)
EXCEPTION(out_of_bounds_error)
EXCEPTION(qname_error)
EXCEPTION(serialization_error)

#undef EXCEPTION


} // namespace


#endif
