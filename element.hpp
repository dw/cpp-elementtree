#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#if __cplusplus >= 201103L
#   include <initializer_list>
#   include <utility>
#   define ETREE_0X
#endif


// libxml forwards.
struct _xmlNode;
struct _xmlDoc;
struct _xmlXPathCompExpr;


namespace etree {

using std::string;

class AttrMap;
class Element;
class ElementTree;
class QName;


Element SubElement(Element &parent, const QName &qname);
Element fromstring(const char *s);
Element fromstring(const string &s);
string tostring(const Element &e);
ElementTree parse(std::istream &is);
ElementTree parse(const string &path);
ElementTree parse(int fd);

std::ostream &operator<< (std::ostream &out, const ElementTree &elem);
std::ostream &operator<< (std::ostream &out, const Element &elem);
std::ostream &operator<< (std::ostream &out, const QName &qname);

#ifdef ETREE_0X
typedef std::pair<string, string> kv_pair;
typedef std::initializer_list<kv_pair> kv_list;
Element SubElement(Element &parent, const QName &qname, kv_list attribs);
#endif


template<typename T>
class Nullable {
    char val_[sizeof(T)];
    bool set_;

    public:
    Nullable();
    Nullable(const T &val);
    Nullable(const Nullable<T> &val);
    #ifdef ETREE_0X
    Nullable(T &&val);
    #endif
    ~Nullable();
    operator bool() const;
    T &operator *();
    const T &operator *() const;
};

typedef Nullable<Element> NullableElement;


class QName {
    string ns_;
    string tag_;

    void from_string(const string &qname);

    public:
    QName(const string &ns, const string &tag);
    QName(const QName &other);
    QName(const string &qname);
    QName(const char *qname);

    string tostring() const;
    const string &tag() const;
    const string &ns() const;
    bool operator==(const QName &other);
};


class XPath {
    _xmlXPathCompExpr *expr_;
    string s_;

    public:
    ~XPath();
    XPath(const char *s);
    XPath(const string &s);
    XPath(const XPath &other);
    const string &expr() const;
    XPath &operator =(const XPath &other);

    Nullable<Element> find(const Element &e) const;
    string findtext(const Element &e) const;
    std::vector<Element> findall(const Element &e) const;
};


class AttrIterator
{
    _xmlNode *node_;

    public:
    AttrIterator(_xmlNode *elem);
    QName key();
    string value();
    bool next();
};


class AttrMap
{
    _xmlNode *node_;

    public:
    ~AttrMap();
    AttrMap(_xmlNode *elem);

    bool has(const QName &qname) const;
    string get(const QName &qname, const string &default_="") const;
    void set(const QName &qname, const string &s);
    std::vector<QName> keys() const;
};


class ElementTree
{
    template<typename P, typename T>
    friend P nodeFor__(const T &);

    _xmlDoc *node_;

    public:
    ~ElementTree();
    ElementTree();
    ElementTree(_xmlDoc *doc);
    Element getroot() const;
};


class Element
{
    template<typename P, typename T>
    friend P nodeFor__(const T &);

    _xmlNode *node_;

    // Never defined.
    Element();

    public:
    ~Element();
    Element(const Element &e);
    Element(_xmlNode *node);
    Element(const QName &qname);
    #ifdef ETREE_0X
    Element(const QName &qname, kv_list attribs);
    #endif

    QName qname() const;
    void qname(const QName &qname);

    string tag() const;
    void tag(const string &tag);

    string ns() const;
    void ns(const string &ns);

    AttrMap attrib() const;
    string get(const QName &qname, const string &default_="") const;

    size_t size() const;
    Element operator[] (size_t i);
    Element &operator=(const Element&);

    Nullable<Element> child(const QName &qn) const;
    std::vector<Element> children(const QName &qn) const;

    Nullable<Element> find(const XPath &expr) const;
    string findtext(const XPath &expr) const;
    std::vector<Element> findall(const XPath &expr) const;

    bool isIndirectParent(const Element &e);
    void append(Element &e);
    void insert(size_t i, Element &e);
    void remove(Element &e);

    Nullable<Element> getnext() const;
    Nullable<Element> getparent() const;
    Nullable<Element> getprev() const;
    ElementTree getroottree() const;

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
EXCEPTION(internal_error)
EXCEPTION(invalid_xpath_error)
EXCEPTION(memory_error)
EXCEPTION(missing_namespace_error)
EXCEPTION(missing_value_error)
EXCEPTION(out_of_bounds_error)
EXCEPTION(parse_error)
EXCEPTION(qname_error)
EXCEPTION(serialization_error)

#undef EXCEPTION

struct xml_error : public std::runtime_error
{
    xml_error(const char *s) : std::runtime_error(s) {}
};


} // namespace


#endif
