
#include <algorithm>
#include <cstring>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <utility>

#include "deleter.hpp"
#include "uname.hpp"


namespace etree {


class DocProxy;
class NodeProxy;
class Element;
Element SubElement(Element &parent, const UniversalName &uname);


struct element_error : public std::runtime_error
{
    element_error(const char *s) : std::runtime_error(s) {}
};


struct DocProxy
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
        doc->_private = nullptr;
        xmlFreeDoc(doc);
    }

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
        auto proxy = static_cast<DocProxy *>(doc->_private);
        if(proxy) {
            proxy->ref();
        } else {
            proxy = new DocProxy(doc);
        }
        return proxy;
    }
};


struct NodeProxy
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
        node->_private = nullptr;
        doc_proxy->unref();
    }

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
        auto proxy = static_cast<NodeProxy *>(node->_private);
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
    friend Element;

    NodeProxy *proxy_ = 0;
    xmlNodePtr node_;

    AttrMap(xmlNodePtr node) : node_(node)
    {
        if(node_) {
            proxy_ = NodeProxy::ref_node(node_);
        }
    }

    static const xmlChar *c_str(const std::string &s)
    {
        return (const xmlChar *) (s.empty() ? nullptr : s.c_str());
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
            return nullptr;
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
            names.emplace_back(ns, (const char *)p->name);
            p = p->next;
        }
        return names;
    }
};


class Element
{
    NodeProxy *proxy_ = nullptr;
    xmlNodePtr node_ = nullptr;

    public:
    ~Element()
    {
        if(node_) {
            proxy_->unref();
        }
    }

    Element() {}
    Element(const Element &e) : node_(e.node_)
    {
        if(node_) {
            proxy_ = NodeProxy::ref_node(node_);
        }
    }

    Element(Element &&e) : node_(e.node_), proxy_(e.proxy_)
    {
        e.node_ = nullptr;
        e.proxy_ = nullptr;
    }

    Element(xmlNodePtr node) : node_(node) {
        proxy_ = NodeProxy::ref_node(node_);
    }

    Element(const UniversalName &un)
    {
        xmlDocPtr doc = ::xmlNewDoc(nullptr);
        if(doc == nullptr) {
            throw element_error("allocation failed");
        }

        node_ = ::xmlNewDocNode(doc, nullptr,
            (const xmlChar *)(un.tag().c_str()), nullptr);
        if(node_ == nullptr) {
            throw element_error("allocation failed");
        }
        ::xmlDocSetRootElement(doc, node_);
        proxy_ = NodeProxy::ref_node(node_);

        if(un.ns().size()) {
            xmlNsPtr ns = ::xmlNewNs(node_,
                                     (xmlChar *)un.ns().c_str(), nullptr);
            if(ns == nullptr) {
                ::xmlFreeNode(node_);
                node_ = nullptr;
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
        return {ns(), tag()};
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
        return {node_};
    }

    std::string get(const UniversalName &un,
                    const std::string &default_="") const
    {
        return attrib().get(un, default_);
    }


    operator bool() const
    {
        return node_ != nullptr;
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
        return {cur};
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
            return {node_->parent};
        }
        return {};
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
                    return nullptr;
            }
        }
        return nullptr;
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
            assert(text != nullptr);
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
            assert(text != nullptr);
            ::xmlAddNextSibling(node, text);
        }
    }

    public:
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


template<class T>
void dumpVector(const char *s, T v)
{
    int i = 0;
    std::cout << s << ":" << std::endl;
    for(auto e : v) {
        std::cout << "vector " << i++ << ": " << e << std::endl;
    }
    std::cout << std::endl;
}


Element SubElement(Element &parent, const UniversalName &uname)
{
    if(! parent) {
        throw element_error("cannot create sub-element on empty element.");
    }

    Element elem(uname);
    parent.append(elem);
    return elem;
}


} // namespace
