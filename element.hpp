
#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cassert>
#include <utility>

#include "exceptions.hpp"


namespace etree {


class AttrMap;
class DocProxy;
class Element;
class ElementTree;
class NodeProxy;
class UniversalName;

Element SubElement(Element &parent, const UniversalName &uname);
Element fromstring(const std::string &s);
std::string tostring(const Element &e);
ElementTree parse(FILE *fp);
ElementTree parse(const std::istream &is);
ElementTree parse(const std::string &path);
ElementTree parse(int fd);

std::ostream &operator<< (std::ostream &out, const Element &elem);
std::ostream &operator<< (std::ostream &out, const UniversalName &un);


class UniversalName {
    std::string ns_;
    std::string tag_;

    void from_string(const std::string &uname);

    public:
    UniversalName(const std::string &ns, const std::string &tag);
    UniversalName(const UniversalName &other);
    UniversalName(const std::string &uname);
    UniversalName(const char *uname);

    const std::string &tag() const;
    const std::string &ns() const;
    bool operator=(const UniversalName &other);
};


class AttrMap
{
    NodeProxy *proxy_;

    public:
    ~AttrMap();
    AttrMap(NodeProxy *proxy);

    bool has(const UniversalName &un) const;
    std::string get(const UniversalName &un,
                    const std::string &default_="") const;
    void set(const UniversalName &un, const std::string &s);
    std::vector<UniversalName> keys() const;
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
    Element(const UniversalName &un);

    #if __cplusplus >= 201103L
    Element(Element &&e);
    #endif

    size_t size() const;
    UniversalName uname() const;
    const char *tag() const;
    const char *ns() const;
    AttrMap attrib() const;
    std::string get(const UniversalName &un,
                    const std::string &default_="") const;
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


} // namespace


#endif
