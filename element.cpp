
/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>

#include "element.hpp"


namespace etree {


/// -----------------------
/// Nullable implementation
/// -----------------------

template<typename T>
Nullable<T>::Nullable()
    : set_(false)
{
}


template<typename T>
Nullable<T>::Nullable(const T &val)
    : set_(true)
{
    new (reinterpret_cast<T *>(val_)) T(val);
}


template<typename T>
Nullable<T>::Nullable(const Nullable<T> &val)
    : set_(val.set_)
{
    if(set_) {
        **this = *val;
    }
}


#if __cplusplus >= 201103L
template<typename T>
Nullable<T>::Nullable(T &&val)
    : set_(true)
{
    new (reinterpret_cast<T *>(val_)) T(val);
}
#endif


template<typename T>
Nullable<T>::~Nullable()
{
    if(set_) {
        T &val = *reinterpret_cast<T *>(val_);
        val.~T();
    }
}


template<typename T>
Nullable<T>::operator bool() const
{
    return set_;
}


template<typename T>
T &Nullable<T>::operator *()
{
    if(! set_) {
        throw missing_value_error();
    }
    return *reinterpret_cast<T *>(val_);
}


template<typename T>
const T &Nullable<T>::operator *() const
{
    if(! set_) {
        throw missing_value_error();
    }
    return *reinterpret_cast<const T *>(val_);
}


// Instantiations.
template class Nullable<Element>;
template class Nullable<std::string>;


/// --------------------------------------
/// libxml2 DOM reference counting classes
/// --------------------------------------

template<typename T, typename Q>
struct ProxyBase
{
    unsigned int refs;
    T dom;

    ProxyBase(T dom)
        : refs(1)
        , dom(dom)
    {
        assert(! dom->_private);
        dom->_private = static_cast<void *>(this);
    }

    virtual ~ProxyBase()
    {
    }

    void ref()
    {
        refs++;
    }

    void unref()
    {
        if(! --refs) {
            delete this;
        }
    }

    static Q *get(T dom)
    {
        assert(dom);
        Q *proxy = static_cast<Q *>(dom->_private);
        if(proxy) {
            proxy->ref();
        } else {
            proxy = new Q(dom);
        }
        return proxy;
    }
};


struct DocProxy
    : public ProxyBase<xmlDocPtr, DocProxy>
{
    DocProxy(xmlDocPtr dom)
        : ProxyBase(dom)
    {
    }

    virtual ~DocProxy()
    {
        dom->_private = 0;
        xmlFreeDoc(dom);
    }
};


struct NodeProxy
    : public ProxyBase<xmlNodePtr, NodeProxy>
{
    DocProxy *doc_proxy;

    NodeProxy(xmlNodePtr dom)
        : ProxyBase(dom)
    {
        doc_proxy = DocProxy::get(dom->doc);
    }

    virtual ~NodeProxy()
    {
        dom->_private = 0;
        doc_proxy->unref();
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


void QName::from_string(const std::string &qname)
{
    if(qname.size() > 0 && qname[0] == '{') {
        size_t e = qname.find('}');
        if(e == std::string::npos) {
            throw qname_error();
        } else if(qname.size() - 1 == e) {
            throw qname_error();
        }
        ns_ = qname.substr(1, e - 1);
        tag_ = qname.substr(e + 1);
        if(tag_.size() == 0) {
            throw qname_error();
        }
    } else {
        ns_ = "";
        tag_ = qname;
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


QName::QName(const std::string &qname)
{
    from_string(qname);
}


QName::QName(const char *qname)
{
    from_string(qname);
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


/// -----------------
/// AttrMap iterators
/// -----------------

AttrIterator::AttrIterator(NodeProxy *proxy)
    : proxy_(proxy)
    , state(-1)
{
}


QName AttrIterator::key()
{
    const char *ns = "";
    if(proxy_->dom->nsDef) {
        ns = reinterpret_cast<const char *>(proxy_->dom->nsDef->href);
    }
    return QName(ns, reinterpret_cast<const char *>(proxy_->dom->name));
}


std::string AttrIterator::value()
{
    std::string out;
    xmlChar *s = ::xmlNodeListGetString(proxy_->dom->doc,
                                        proxy_->dom->children, 1);
    if(s) {
        out = reinterpret_cast<const char *>(s);
        ::xmlFree(s);
    }

    return out;
}


bool AttrIterator::next()
{
    if(! proxy_->dom->next) {
        return false;
    }
    NodeProxy *new_ = NodeProxy::get(proxy_->dom->next);
    proxy_->unref();
    proxy_ = new_;
    return true;
}


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
    return ::xmlHasNsProp(proxy_->dom, c_str(un.tag()), c_str(un.ns()));
}


std::string AttrMap::get(const QName &un,
                         const std::string &default_) const
{
    if(! proxy_) {
        return default_;
    }
    std::string out(default_);
    const char *s = (const char *)
        ::xmlGetNsProp(proxy_->dom, c_str(un.tag()), c_str(un.ns()));

    if(s) {
        out = s;
        ::xmlFree((void *) s);
    }
    return out;
}


void AttrMap::set(const QName &un, const std::string &s)
{
    if(proxy_) {
        ::xmlSetNsProp(proxy_->dom,
           findNs_(proxy_->dom, un.ns()), c_str(un.tag()), c_str(s));
    }
}


std::vector<QName> AttrMap::keys() const
{
    std::vector<QName> names;
    if(! proxy_) {
        return names;
    }
    xmlAttrPtr p = proxy_->dom->properties;
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


ElementTree::ElementTree(DocProxy *proxy)
    : proxy_(proxy)
{
}


ElementTree::operator bool() const
{
    return proxy_ != 0;
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
    proxy_ = NodeProxy::get(node);

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
    return ::xmlChildElementCount(proxy_->dom);
}


QName Element::qname() const
{
    return QName(ns(), tag());
}


const char *Element::tag() const
{
    return proxy_ ? (const char *) proxy_->dom->name : "";
}


const char *Element::ns() const
{
    if(proxy_ && proxy_->dom->nsDef) {
        return (const char *) proxy_->dom->nsDef->href;
    }
    return "";
}


AttrMap Element::attrib() const
{
    return AttrMap(proxy_);
}


std::string Element::get(const QName &un, const std::string &default_) const
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

    xmlNodePtr cur = proxy_->dom->children;
    while(i) {
        if(cur->type == XML_ELEMENT_NODE) {
            i--;
        }
        cur = cur->next;
        if(! cur) {
            throw out_of_bounds_error();
        }
    }
    return Element(NodeProxy::get(cur));
}


bool Element::isIndirectParent(const Element &e)
{
    if(proxy_ && e.proxy_) {
        xmlNodePtr other = e.proxy_->dom;
        xmlNodePtr parent = proxy_->dom->parent;
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
        std::cout << "appending " << e.proxy_->dom;
        std::cout << " to " << proxy_->dom << std::endl;
        if(isIndirectParent(e)) {
            throw cyclical_tree_error();
        }
        ::xmlUnlinkNode(e.proxy_->dom);
        ::xmlAddChild(proxy_->dom, e.proxy_->dom);
    }
}


void Element::insert(size_t i, Element &e)
{
    if(proxy_ && e.proxy_) {
        if(i == size()) {
            append(e);
        } else {
            ::xmlAddPrevSibling(this[i].proxy_->dom, e.proxy_->dom);
        }
    }
}


void Element::remove(Element &e)
{
    if(proxy_ && e.proxy_) {
        if(e.proxy_->dom->parent == proxy_->dom) {
            ::xmlUnlinkNode(e.proxy_->dom);
        }
    }
}


Element Element::getparent() const
{
    if(proxy_->dom && proxy_->dom->parent &&
            proxy_->dom->parent->type != XML_DOCUMENT_NODE) {
        return Element(NodeProxy::get(proxy_->dom->parent));
    }
    return Element();
}


ElementTree Element::getroottree() const
{
    if(proxy_) {
        return ElementTree(DocProxy::get(proxy_->dom->doc));
    }
    return ElementTree();
}


NodeProxy *Element::proxy() const
{
    return proxy_;
}


std::string Element::text() const
{
    return proxy_ ? _collectText(proxy_->dom->children) : "";
}


void Element::text(const std::string &s)
{
    if(proxy_) {
        _setNodeText(proxy_->dom, s);
    }
}


std::string Element::tail() const
{
    return proxy_ ? _collectText(proxy_->dom->next) : "";
}


void Element::tail(const std::string &s)
{
    if(proxy_) {
        _setTailText(proxy_->dom, s);
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

        int ret = ::xmlSaveTree(ctx, proxy->dom);
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

Element SubElement(Element &parent, const QName &qname)
{
    if(! parent) {
        throw empty_element_error();
    }

    Element elem(qname);
    parent.append(elem);
    return elem;
}


Element fromstring(const std::string &s)
{
    xmlDocPtr doc = ::xmlReadMemory(s.data(), s.size(), 0, 0, 0);
    if(! (doc && doc->children)) {
        return Element();
    }

    return Element(NodeProxy::get(doc->children));
}


Element XML(const std::string &s)
{
    return fromstring(s);
}


/// ----------------------
/// parse() implementation
/// ----------------------

static int DUMMY_close(void *ignored)
{
    return 0;
}


int istream_read___(void *strm, char *buffer, int len)
{
    std::istream &is = *static_cast<std::istream *>(strm);

    is.read(buffer, len);
    if(is.fail() && !is.eof()) {
        return -1;
    }
    return is.gcount();
}


template<int(*fn)(void *, char *, int), typename T>
static ElementTree parse_internal(T obj)
{
    xmlDocPtr doc = ::xmlReadIO(fn, DUMMY_close,
                                static_cast<void *>(obj), 0, 0, 0);
    if(doc) {
        return ElementTree(DocProxy::get(doc));
    }
    return ElementTree();
}


ElementTree parse(std::istream &is)
{
    return parse_internal<istream_read___>(&is);
}


/// -----------------
/// iostreams support
/// -----------------


std::ostream &operator<< (std::ostream &out, const ElementTree &tree)
{
    if(tree) {
        out << "<ElementTree>";
    } else {
        out << "<invalid ElementTree>";
    }
    return out;
}


std::ostream &operator<< (std::ostream &out, const Element &elem)
{
    if(elem) {
        out << "<Element " << elem.tag() << " with " << elem.size();
        out << " children>";
    } else {
        out << "<invalid Element>";
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
