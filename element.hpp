
#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

#include <iostream>
#include <algorithm>
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
Element SubElement(Element &parent, const UniversalName &uname);
Element fromstring(const std::string &s);
std::string tostring(const Element &e);


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


class Element
{
    NodeProxy *proxy_;
    xmlNodePtr node_;

    public:
    ~Element()
    {
        if(node_) {
            proxy_->unref();
        }
    }

    Element() : proxy_(0), node_(0) {}
    Element(const Element &e) : node_(e.node_)
    {
        proxy_ = node_ ? NodeProxy::ref_node(node_) : 0;
    }

    /*
    Element(Element &&e) : node_(e.node_), proxy_(e.proxy_)
    {
        e.node_ = 0;
        e.proxy_ = 0;
    }
    */

    Element(xmlNodePtr node) : node_(node) {
        proxy_ = NodeProxy::ref_node(node_);
    }

    Element(const UniversalName &un)
    {
        xmlDocPtr doc = ::xmlNewDoc(0);
        if(doc == 0) {
            throw element_error("allocation failed");
        }

        node_ = ::xmlNewDocNode(doc, 0,
            (const xmlChar *)(un.tag().c_str()), 0);
        if(node_ == 0) {
            throw element_error("allocation failed");
        }
        ::xmlDocSetRootElement(doc, node_);
        proxy_ = NodeProxy::ref_node(node_);

        if(un.ns().size()) {
            xmlNsPtr ns = ::xmlNewNs(node_,
                                     (xmlChar *)un.ns().c_str(), 0);
            if(ns == 0) {
                ::xmlFreeNode(node_);
                node_ = 0;
                throw element_error("allocation failed");
            }
            ::xmlSetNs(node_, ns);
        }
    }

    size_t size() const
    {
        return ::xmlChildElementCount(node_);
    }

    UniversalName uname() const
    {
        return UniversalName(ns(), tag());
    }

    const char *tag() const
    {
        return node_ ? (const char *) node_->name : "";
    }

    const char *ns() const
    {
        if(node_ && node_->nsDef) {
            return (const char *) node_->nsDef->href;
        }
        return "";
    }

    // -------
    // Attributes
    // -------

    AttrMap attrib() const
    {
        return AttrMap(node_);
    }

    std::string get(const UniversalName &un,
                    const std::string &default_="") const
    {
        return attrib().get(un, default_);
    }


    operator bool() const
    {
        return node_ != 0;
    }

    Element operator[] (size_t i)
    {
        xmlNodePtr cur = node_->children;
        while(i) {
            if(cur->type == XML_ELEMENT_NODE) {
                i--;
            }
            cur = cur->next;
            if(! cur) {
                throw element_error("operator[] out of bounds");
            }
        }
        return Element(cur);
    }

    bool isIndirectParent(const Element &e)
    {
        if(node_) {
            xmlNodePtr parent = node_->parent;
            while(parent) {
                if(parent == e.node_) {
                    return true;
                }
                parent = parent->parent;
            }
        }
        return false;
    }

    void append(Element &e)
    {
        std::cout << "appending " << e.node_ << " to " << node_ << std::endl;
        if((! *this) || isIndirectParent(e)) {
            throw element_error("cannot append indirect parent to child");
        }
        ::xmlUnlinkNode(e.node_);
        ::xmlAddChild(node_, e.node_);
    }

    void insert(size_t i, Element &e)
    {
        if(i == size()) {
            append(e);
        } else {
            ::xmlAddPrevSibling(this[i].node_, e.node_);
        }
    }

    void remove(Element &e)
    {
        if(e.node_->parent == node_) {
            ::xmlUnlinkNode(e.node_);
        }
    }

    Element getparent() const
    {
        if(node_->parent && node_->parent->type != XML_DOCUMENT_NODE) {
            return Element(node_->parent);
        }
        return Element();
    }

    //
    // Text functions
    //

    protected:
    xmlNodePtr _textNodeOrSkip(xmlNodePtr node) const
    {
        while(node) {
            switch(node->type) {
                case XML_TEXT_NODE:
                case XML_CDATA_SECTION_NODE:
                    return node;
                case XML_XINCLUDE_START:
                case XML_XINCLUDE_END:
                    node = node->next;
                    break;
                default:
                    return 0;
            }
        }
        return 0;
    }

    std::string _collectText(xmlNodePtr node) const
    {
        std::string result;
        while(node) {
            result += (const char *) node->content;
            node = _textNodeOrSkip(node);
        }
        return result;
    }

    void _removeText(xmlNodePtr node)
    {
        node = _textNodeOrSkip(node);
        while(node) {
            xmlNodePtr next = _textNodeOrSkip(node);
            ::xmlUnlinkNode(node);
            ::xmlFreeNode(node);
            node = next;
        }
    }

    void _setNodeText(xmlNodePtr node, const std::string &s)
    {
        _removeText(node->children);
        // TODO: CDATA
        if(s.size()) {
            xmlNodePtr text = ::xmlNewText((xmlChar *) s.c_str());
            assert(text != 0);
            if(node->children) {
                ::xmlAddPrevSibling(node->children, text);
            } else {
                ::xmlAddChild(node, text);
            }
        }
    }

    void _setTailText(xmlNodePtr node, const std::string &s)
    {
        _removeText(node->next);
        if(s.size()) {
            xmlNodePtr text = ::xmlNewText((xmlChar *) s.c_str());
            assert(text != 0);
            ::xmlAddNextSibling(node, text);
        }
    }

    public:
    xmlNodePtr _node() const
    {
        return node_;
    }

    std::string text() const
    {
        return node_ ? _collectText(node_->children) : "";
    }

    void text(const std::string &s)
    {
        if(node_) {
            _setNodeText(node_, s);
        }
    }

    std::string tail() const
    {
        return node_ ? _collectText(node_->next) : "";
    }

    void tail(const std::string &s)
    {
        if(node_) {
            _setTailText(node_, s);
        }
    }
};


} // namespace


#endif
