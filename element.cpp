
#include <cassert>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

#include "element.hpp"


namespace etree {


/// --------------------------------------
/// libxml2 DOM reference counting classes
/// --------------------------------------

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
        doc->_private = 0;
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
        DocProxy *proxy = static_cast<DocProxy *>(doc->_private);
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
        node->_private = 0;
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
        NodeProxy *proxy = static_cast<NodeProxy *>(node->_private);
        if(proxy) {
            proxy->ref();
        } else {
            proxy = new NodeProxy(node);
        }
        return proxy;
    }
};


/// -------------------------
/// Internal helper functions
/// -------------------------


static const xmlChar *c_str(const std::string &s)
{
    return (const xmlChar *) (s.empty() ? 0 : s.c_str());
}


static xmlNodePtr _textNodeOrSkip(xmlNodePtr node)
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


static void _removeText(xmlNodePtr node)
{
    node = _textNodeOrSkip(node);
    while(node) {
        xmlNodePtr next = _textNodeOrSkip(node);
        ::xmlUnlinkNode(node);
        ::xmlFreeNode(node);
        node = next;
    }
}

static void _setNodeText(xmlNodePtr node, const std::string &s)
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

static void _setTailText(xmlNodePtr node, const std::string &s)
{
    _removeText(node->next);
    if(s.size()) {
        xmlNodePtr text = ::xmlNewText((xmlChar *) s.c_str());
        assert(text != 0);
        ::xmlAddNextSibling(node, text);
    }
}


static std::string _collectText(xmlNodePtr node)
{
    std::string result;
    while(node) {
        result += (const char *) node->content;
        node = _textNodeOrSkip(node);
    }
    return result;
}


static const xmlNsPtr findNs_(xmlNodePtr node, const std::string &ns)
{
    if(! ns.size()) {
        return 0;
    }
    xmlNsPtr p = node->nsDef;
    while(p) {
        if(ns == (const char *) p->href) {
            return p;
        }
    }
    throw missing_namespace_error();
}


/// ---------------
/// QName functions
/// ---------------


void QName::from_string(const std::string &uname)
{
    if(uname.size() > 0 && uname[0] == '{') {
        size_t e = uname.find('}');
        if(e == std::string::npos) {
            throw qname_error();
        } else if(uname.size() - 1 == e) {
            throw qname_error();
        }
        ns_ = uname.substr(1, e - 1);
        tag_ = uname.substr(e + 1);
        if(tag_.size() == 0) {
            throw qname_error();
        }
    } else {
        ns_ = "";
        tag_ = uname;
    }
}


QName::QName(const std::string &ns, const std::string &tag)
    : ns_(ns)
    , tag_(tag)
{
}


QName::QName(const QName &other)
    : ns_(other.ns_)
    , tag_(other.tag_)
{
}


QName::QName(const std::string &uname)
{
    from_string(uname);
}


QName::QName(const char *uname)
{
    from_string(uname);
}


const std::string &QName::tag() const
{
    return tag_;
}


const std::string &QName::ns() const
{
    return ns_;
}


bool QName::operator=(const QName &other)
{
    return other.tag_ == tag_ && other.ns_ == ns_;
}


typedef int dave;
typedef int dave;


/// -----------------
/// AttrMap functions
/// -----------------

AttrMap::AttrMap(NodeProxy *proxy)
    : proxy_(proxy)
{
    if(proxy_) {
        proxy_->ref();
    }
}


AttrMap::~AttrMap()
{
    if(proxy_) {
        proxy_->unref();
    }
}


bool AttrMap::has(const QName &un) const
{
    if(! proxy_) {
        return false;
    }
    return ::xmlHasNsProp(proxy_->node, c_str(un.tag()), c_str(un.ns()));
}


std::string AttrMap::get(const QName &un,
                         const std::string &default_) const
{
    if(! proxy_) {
        return default_;
    }
    std::string out(default_);
    const char *s = (const char *)
        ::xmlGetNsProp(proxy_->node, c_str(un.tag()), c_str(un.ns()));

    if(s) {
        out = s;
        ::xmlFree((void *) s);
    }
    return out;
}


void AttrMap::set(const QName &un, const std::string &s)
{
    if(proxy_) {
        ::xmlSetNsProp(proxy_->node,
           findNs_(proxy_->node, un.ns()), c_str(un.tag()), c_str(s));
    }
}


std::vector<QName> AttrMap::keys() const
{
    std::vector<QName> names;
    if(! proxy_) {
        return names;
    }
    xmlAttrPtr p = proxy_->node->properties;
    while(p) {
        const char *ns = p->ns ? (const char *)p->ns->href : "";
        names.push_back(QName(ns, (const char *)p->name));
        p = p->next;
    }
    return names;
}


/// ---------------------
/// ElementTree functions
/// ---------------------

ElementTree::~ElementTree()
{
    if(proxy_) {
        proxy_->unref();
    }
}


ElementTree::ElementTree()
    : proxy_(0)
{
}




/// -----------------
/// Element functions
/// -----------------

Element::~Element()
{
    if(proxy_) {
        proxy_->unref();
    }
}


Element::Element()
    : proxy_(0)
{
}


Element::Element(const Element &e)
    : proxy_(e.proxy_)
{
    if(proxy_) {
        proxy_->ref();
    }
}


#if __cplusplus >= 201103L
Element::Element(Element &&e)
    : proxy_(e.proxy_)
{
    e.proxy_ = 0;
}
#endif


Element::Element(NodeProxy *proxy)
    : proxy_(proxy)
{
}


Element::Element(const QName &un)
{
    xmlDocPtr doc = ::xmlNewDoc(0);
    if(doc == 0) {
        throw memory_error();
    }

    xmlNodePtr node = ::xmlNewDocNode(doc, 0,
        (const xmlChar *)(un.tag().c_str()), 0);
    if(node == 0) {
        throw memory_error();
    }
    ::xmlDocSetRootElement(doc, node);
    proxy_ = NodeProxy::ref_node(node);

    if(un.ns().size()) {
        xmlNsPtr ns = ::xmlNewNs(node, (xmlChar *)un.ns().c_str(), 0);
        if(ns == 0) {
            ::xmlFreeNode(node);
            throw memory_error();
        }
        ::xmlSetNs(node, ns);
    }
}


size_t Element::size() const
{
    return ::xmlChildElementCount(proxy_->node);
}


QName Element::uname() const
{
    return QName(ns(), tag());
}


const char *Element::tag() const
{
    return proxy_ ? (const char *) proxy_->node->name : "";
}


const char *Element::ns() const
{
    if(proxy_ && proxy_->node->nsDef) {
        return (const char *) proxy_->node->nsDef->href;
    }
    return "";
}


AttrMap Element::attrib() const
{
    return AttrMap(proxy_);
}


std::string Element::get(const QName &un,
                         const std::string &default_) const
{
    return attrib().get(un, default_);
}


Element::operator bool() const
{
    return proxy_ != 0;
}


Element Element::operator[] (size_t i)
{
    if(! proxy_) {
        return Element();
    }

    xmlNodePtr cur = proxy_->node->children;
    while(i) {
        if(cur->type == XML_ELEMENT_NODE) {
            i--;
        }
        cur = cur->next;
        if(! cur) {
            throw out_of_bounds_error();
        }
    }
    return Element(NodeProxy::ref_node(cur));
}


bool Element::isIndirectParent(const Element &e)
{
    if(proxy_ && e.proxy_) {
        xmlNodePtr other = e.proxy_->node;
        xmlNodePtr parent = proxy_->node->parent;
        while(parent) {
            if(parent == other) {
                return true;
            }
            parent = parent->parent;
        }
    }
    return false;
}


void Element::append(Element &e)
{
    if(proxy_ && e.proxy_) {
        std::cout << "appending " << e.proxy_->node;
        std::cout << " to " << proxy_->node << std::endl;
        if(isIndirectParent(e)) {
            throw cyclical_tree_error();
        }
        ::xmlUnlinkNode(e.proxy_->node);
        ::xmlAddChild(proxy_->node, e.proxy_->node);
    }
}


void Element::insert(size_t i, Element &e)
{
    if(proxy_ && e.proxy_) {
        if(i == size()) {
            append(e);
        } else {
            ::xmlAddPrevSibling(this[i].proxy_->node, e.proxy_->node);
        }
    }
}


void Element::remove(Element &e)
{
    if(proxy_ && e.proxy_) {
        if(e.proxy_->node->parent == proxy_->node) {
            ::xmlUnlinkNode(e.proxy_->node);
        }
    }
}


Element Element::getparent() const
{
    if(proxy_->node && proxy_->node->parent &&
            proxy_->node->parent->type != XML_DOCUMENT_NODE) {
        return Element(NodeProxy::ref_node(proxy_->node->parent));
    }
    return Element();
}


NodeProxy *Element::proxy() const
{
    return proxy_;
}


std::string Element::text() const
{
    return proxy_ ? _collectText(proxy_->node->children) : "";
}


void Element::text(const std::string &s)
{
    if(proxy_) {
        _setNodeText(proxy_->node, s);
    }
}


std::string Element::tail() const
{
    return proxy_ ? _collectText(proxy_->node->next) : "";
}


void Element::tail(const std::string &s)
{
    if(proxy_) {
        _setTailText(proxy_->node, s);
    }
}


/// -------------------------
/// tostring() implementation
/// -------------------------


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
    NodeProxy *proxy = e.proxy();
    std::string out;
    if(proxy) {
        xmlSaveCtxtPtr ctx = ::xmlSaveToIO(writeCallback, closeCallback,
            static_cast<void *>(&out), 0, 0);

        int ret = ::xmlSaveTree(ctx, proxy->node);
        ::xmlSaveClose(ctx);
        if(ret == -1) {
            throw serialization_error();
        }
    }
    return out;
}


/// ----------------
/// Helper functions
/// ----------------

Element SubElement(Element &parent, const QName &uname)
{
    if(! parent) {
        throw empty_element_error();
    }

    Element elem(uname);
    parent.append(elem);
    return elem;
}


Element fromstring(const std::string &s)
{
    xmlDocPtr doc = ::xmlReadMemory(s.data(), s.size(), 0, 0, 0);
    if(! (doc && doc->children)) {
        return Element();
    }

    return Element(NodeProxy::ref_node(doc->children));
}


/// ----------------------
/// parse() implementation
/// ----------------------




/// -----------------
/// iostreams support
/// -----------------


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


std::ostream& operator<< (std::ostream& out, const QName& un)
{
    if(un.ns().size()) {
        out << "{" << un.ns() << "}";
    }
    out << un.tag();
    return out;
}


} // namespace
