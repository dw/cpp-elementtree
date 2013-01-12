
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
#include "uname.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>


namespace etree {


class Element;
class ElementTree;
Element SubElement(Element &parent, const UniversalName &uname);
Element fromstring(const std::string &s);
std::string tostring(const Element &e);
ElementTree parse(const std::string &path);
ElementTree parse(int fd);
ElementTree parse(FILE *fp);
ElementTree parse(const std::istream &is);



class DocProxy
{
    xmlDocPtr doc;
    unsigned int refs;

    DocProxy(xmlDocPtr doc) : doc(doc), refs(1)
    {
        assert(! doc->_private);
        doc->_private = static_cast<void *>(this);
    }

    ~DocProxy()
    {
        doc->_private = 0;
        xmlFreeDoc(doc);
    }

    public:
    void ref()
    {
        refs++;
        std::cout << "!! DocProxy::ref() now " << refs << std::endl;
    }

    void unref()
    {
        refs--;
        std::cout << "!! DocProxy::unref() now " << refs << std::endl;
        if(! refs) {
            std::cout << "@@ Deleting " << this << std::endl;
            delete this;
        }
    }

    static DocProxy *ref_doc(xmlDocPtr doc)
    {
        assert(doc);
        DocProxy *proxy = static_cast<DocProxy *>(doc->_private);
        if(proxy) {
            proxy->ref();
        } else {
            proxy = new DocProxy(doc);
        }
        return proxy;
    }
};


class NodeProxy
{
    xmlNodePtr node;
    DocProxy *doc_proxy;
    unsigned int refs;

    NodeProxy(xmlNodePtr node) : node(node), refs(1)
    {
        assert(! node->_private);
        node->_private = static_cast<void *>(this);
        doc_proxy = DocProxy::ref_doc(node->doc);
    }

    ~NodeProxy()
    {
        node->_private = 0;
        doc_proxy->unref();
    }

    public:
    void ref()
    {
        assert(refs < 100);
        refs++;
        std::cout << "!! NodeProxy::ref() now " << refs << std::endl;
    }

    void unref()
    {
        refs--;
        std::cout << "!! NodeProxy::unref() now " << refs << std::endl;
        if(! refs) {
            std::cout << "@@ Deleting " << this << std::endl;
            delete this;
        }
    }

    static NodeProxy *ref_node(xmlNodePtr node)
    {
        assert(node);
        NodeProxy *proxy = static_cast<NodeProxy *>(node->_private);
        if(proxy) {
            proxy->ref();
        } else {
            proxy = new NodeProxy(node);
        }
        return proxy;
    }
};


class AttrMap
{
    friend class Element;

    NodeProxy *proxy_;
    xmlNodePtr node_;

    AttrMap(xmlNodePtr node) : node_(node)
    {
        proxy_ = node_ ? NodeProxy::ref_node(node_) : 0;
    }

    static const xmlChar *c_str(const std::string &s)
    {
        return (const xmlChar *) (s.empty() ? 0 : s.c_str());
    }

    public:
    ~AttrMap()
    {
        if(node_) {
            proxy_->unref();
        }
    }

    bool has(const UniversalName &un) const
    {
        if(! node_) {
            return false;
        }
        return ::xmlHasNsProp(node_, c_str(un.tag()), c_str(un.ns()));
    }

    std::string get(const UniversalName &un,
                    const std::string &default_="") const
    {
        if(! node_) {
            return default_;
        }
        std::string out(default_);
        const char *s = (const char *) xmlGetNsProp(node_, c_str(un.tag()),
                                                           c_str(un.ns()));
        if(s) {
            out = s;
            ::xmlFree((void *) s);
        }
        return out;
    }

    const xmlNsPtr findNs_(const std::string &ns)
    {
        if(! ns.size()) {
            return 0;
        }
        xmlNsPtr p = node_->nsDef;
        while(p) {
            if(ns == (const char *) p->href) {
                return p;
            }
        }
        throw element_error("could not find ns");
    }

    void set(const UniversalName &un, const std::string &s)
    {
        if(node_) {
            ::xmlSetNsProp(node_, findNs_(un.ns()), c_str(un.tag()), c_str(s));
        }
    }

    std::vector<UniversalName> keys() const
    {
        std::vector<UniversalName> names;
        if(! node_) {
            return names;
        }
        xmlAttrPtr p = node_->properties;
        while(p) {
            const char *ns = p->ns ? (const char *)p->ns->href : "";
            names.push_back(UniversalName(ns, (const char *)p->name));
            p = p->next;
        }
        return names;
    }
};


class ElementTree
{
    DocProxy *proxy_;
    xmlDocPtr doc_;

    public:
    ~ElementTree();
    ElementTree();
};


class Element
{
    NodeProxy *proxy_;
    xmlNodePtr node_;

    public:
    ~Element();
    Element();
    Element(const Element &e);
    Element(xmlNodePtr node);
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

    public:
    xmlNodePtr _node() const;
    std::string text() const;
    void text(const std::string &s);
    std::string tail() const;
    void tail(const std::string &s);
};


} // namespace


#endif
