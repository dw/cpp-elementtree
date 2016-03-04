/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio> // snprintf().
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>
#include <unistd.h>

#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "elementtree/element.hpp"


namespace etree {


using std::string;
using std::ostream;

#define DEBUG(x, ...) \
    fprintf(stderr, "element.cpp: " x "\n", ## __VA_ARGS__);


// ------------------------------------
// Ensure libxml2 is cleaned up at exit
// ------------------------------------


static struct _atexit_libxml_cleanup {
    _atexit_libxml_cleanup() {
        atexit(xmlCleanupParser);
    }
} _atexit_libxml_cleanup;


// -----------------------
// Nullable implementation
// -----------------------


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
        new (reinterpret_cast<T *>(val_)) T(*val);
    }
}


#ifdef ETREE_0X
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
        reinterpret_cast<T *>(val_)->~T();
    }
}


template<typename T>
bool Nullable<T>::operator==(const Nullable<T> &other) const
{
    if(set_ && (set_ == other.set_)) {
        return **this == *other;
    }
    return set_ == other.set_;
}


template<typename T>
bool Nullable<T>::operator==(const T &other) const
{
    if(! set_) {
        return false;
    }
    return other == **this;
}


template<typename T>
Nullable<T> &Nullable<T>::operator=(const Nullable<T> &other)
{
    if(this != &other) {
        this->~Nullable();
        set_ = other.set_;
        if(set_) {
            new (reinterpret_cast<T *>(val_)) T(*other);
        }
    }
    return *this;
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
T *Nullable<T>::operator ->()
{
    return &**this;
}


template<typename T>
const T &Nullable<T>::operator *() const
{
    if(! set_) {
        throw missing_value_error();
    }
    return *reinterpret_cast<const T *>(val_);
}


template<typename T>
const T *Nullable<T>::operator ->() const
{
    return &**this;
}


// Instantiations.
template class Nullable<Element>;
template class Nullable<string>;


// ----------------------------------------
// libxml2 DOM reference counting functions
// ----------------------------------------

/*
 * Note: it looks possible to only refcount on the parent document, however
 * when multiple Elements exist for a node and one moves it to a new document,
 * the remaining instances will unref() the wrong document during destruction.
 * Therefore we refcount each node and have a single ref() on its parent
 * document.
 */


template<typename T>
static inline intptr_t &
refCount_(T node) {
    return *reinterpret_cast<intptr_t *>(&(node->_private));
}


static xmlDoc *
ref(xmlDoc *doc)
{
    // Relies on NULL (aka. initial state of _private) being (intptr_t)0, which
    // isn't true on some weird archs.
    assert(doc && (sizeof(void *) >= sizeof(intptr_t)));
    refCount_(doc)++;
    return doc;
}


static void
unref(xmlDoc *doc)
{
    assert(doc && doc->_private);
    if(! --refCount_(doc)) {
        xmlFreeDoc(doc);
    }
}


static xmlNode *
ref(xmlNode *node)
{
    assert(node);
    if(! refCount_(node)++) {
        ref(node->doc);
    }
    return node;
}


static void
unref(xmlNode *node)
{
    assert(node && node->_private);
    if(! --refCount_(node)) {
        unref(node->doc);
    }
}


// -------------------------
// Internal helper functions
// -------------------------

template<typename Function>
static void
visit(bool visitAttrs, xmlNode *node, Function func)
{
    func(node);
    for(xmlNode *child = node->children; child; child = child->next) {
        visit(visitAttrs, child, func);
    }

    if(visitAttrs) {
        for(auto child = node->properties; child; child = child->next) {
            func(reinterpret_cast<xmlNode *>(child));
        }
    }
}


static bool
nextElement_(xmlNode *&p)
{
    for(; p; p = p->next) {
        if(p->type == XML_ELEMENT_NODE) {
            return true;
        }
    }
    return false;
}

static const char *
toChar_(const xmlChar *s)
{
    return reinterpret_cast<const char *>(s);
}


static char *
toChar_(xmlChar *s)
{
    return reinterpret_cast<char *>(s);
}


static const char *
nsToChar_(const xmlNs *ns)
{
    return ns ? toChar_(ns->href) : NULL;
}


static const xmlChar *
toXmlChar_(const char *s)
{
    return reinterpret_cast<const xmlChar *>(s);
}


static void
maybeThrow_()
{
    xmlError *error = ::xmlGetLastError();
    if(error) {
        std::string s(error->message);
        while(s.size() && ::isspace(s[s.size() - 1])) {
            s.pop_back();
        }
        throw xml_error(s.c_str());
    }
}


template<typename P, typename T>
/*static*/ P
nodeFor__(const T &e)
{
    return e.node_;
}


#ifdef ETREE_0X
static void
attrsFromList_(Element &elem, kv_list &attribs)
{
    AttrMap attrs = elem.attrib();
    for(auto &kv : attribs) {
        attrs.set(kv.first, kv.second);
    }
}
#endif


static const xmlChar *
c_str(const string &s)
{
    return toXmlChar_(s.empty() ? 0 : s.c_str());
}


static xmlNs *
makeNs_(xmlNode *node, const string &uri)
{
    xmlChar prefix[6];
    xmlNs *ns;

    for(int i = 0; i <= 1000; i++) {
        ::snprintf(toChar_(prefix), sizeof prefix, "ns%d", i);
        ns = ::xmlSearchNs(node->doc, node, prefix);
        if(! ns) {
            break;
        }
    }

    if(ns) {
        // ns0..1000 in use, something broken.
        throw internal_error();
    }

    ns = ::xmlNewNs(node, toXmlChar_(uri.c_str()), prefix);
    if(! ns) {
        throw memory_error();
    }
    return ns;
}


static xmlNs *
getNs_(xmlNode *node, xmlNode *target, const string &uri)
{
    if(uri.empty()) {
        return 0;
    }

    // Look for existing definition.
    xmlNode *doc_node = reinterpret_cast<xmlNode *>(node->doc);
    for(xmlNode *cur = node; cur && cur != doc_node; cur = cur->parent) {
        for(xmlNs *ns = cur->nsDef; ns != 0; ns = ns->next) {
            if(uri == toChar_(ns->href)) {
                return ns;
            }
        }
    }

    return makeNs_(target, uri);
}


/**
 * Return a node if it is a text or CDATA node, or taking care to skipping
 * forward in the siblings list if any XInclude nodes are encountered.
 *
 * @param node
 *      The node.
 */
template<typename Function>
static void
visitText_(xmlNode *node, Function func)
{
    while(node) {
        xmlNode *next = node->next;
        switch(node->type) {
            case XML_TEXT_NODE:
            case XML_CDATA_SECTION_NODE:
                func(node);
            case XML_XINCLUDE_START:
            case XML_XINCLUDE_END:
                break;
            default:
                return;
        }
        node = next;
    }
}


/**
 * Move any text and CDATA nodes previously occurring immediately after a node
 * to the node's new location. Used to fixup Element::tail() following a node
 * move.
 *
 * @param tail
 *      The value of target->next before it was relinked in its new position.
 * @param target
 *      The moved node, to which any text nodes found starting at `tail` should
 *      be moved after.
 */
static void
moveTail_(xmlNode *tail, xmlNode *target)
{
    visitText_(tail, [&](xmlNode *node) {
        ::xmlAddNextSibling(target, tail);
    });
}


/**
 * Removes namespace declarations from an element that are already defined in
 * its parents.  Does not free the xmlNs's, just prepends them to staleNsList.
 */
static void
reparentNs_(xmlNode *node,
            std::map<xmlNs *, xmlNs *> &nsCache,
            xmlNs *&staleNsList)
{
    xmlNs **nsdef = &node->nsDef;
    while(*nsdef) {
        xmlNs *nsNext = (*nsdef)->next;
        xmlNs *ns = ::xmlSearchNsByHref(node->doc, node->parent, (*nsdef)->href);
        if(! ns) {
            // new namespace href => keep and cache the ns declaration
            nsCache.insert(std::make_pair(*nsdef, *nsdef));
        } else {
            // known namespace href => cache mapping and strip old ns. prepend
            // ns to garbage chain.
            nsCache.insert(std::make_pair(*nsdef, ns));
            (*nsdef)->next = staleNsList;
            staleNsList = *nsdef;
        }
        *nsdef = nsNext;
    }
}


/**
 * Walk all child elements and attributes of a recently relinked node, fixing
 * up their namespace references to point to namespaces existing in the new
 * document, assuming.
 */
static void
reparent_(xmlNode *startNode)
{
    std::map<xmlNs *, xmlNs *> nsCache;
    xmlNs *staleNsList = NULL;

    visit(true, startNode, [&](xmlNode *node) {
        xmlNode *nsParent;
        switch(node->type) {
            case XML_ELEMENT_NODE:
            case XML_COMMENT_NODE:
            case XML_ENTITY_REF_NODE:
            case XML_PI_NODE:
            case XML_XINCLUDE_START:
            case XML_XINCLUDE_END:
                reparentNs_(node, nsCache, staleNsList);
                nsParent = node;
                break;
            case XML_ATTRIBUTE_NODE:
                nsParent = node->parent;
                break;
            default:
                return;
        }

        if(node->ns) {
            auto it = nsCache.find(node->ns);
            if(it != nsCache.end()) {
                node->ns = it->second;
            } else {
                xmlNs *oldNs = node->ns;
                node->ns = getNs_(nsParent, startNode, toChar_(node->ns->href));
                nsCache.insert(std::make_pair(oldNs, node->ns));
            }
        }
    });

    if(staleNsList) {
        ::xmlFreeNsList(staleNsList);
    }
}


static void
_removeText(xmlNode *node)
{
    visitText_(node, [&](xmlNode *node) {
        ::xmlUnlinkNode(node);
        ::xmlFreeNode(node);
    });
}


static void
_setNodeText(xmlNode *node, const string &s)
{
    _removeText(node->children);
    // TODO: CDATA
    if(s.size()) {
        xmlNode *text = ::xmlNewText((xmlChar *) s.c_str());
        assert(text != 0);
        if(node->children) {
            ::xmlAddPrevSibling(node->children, text);
        } else {
            ::xmlAddChild(node, text);
        }
    }
}


static void
_setTailText(xmlNode *node, const string &s)
{
    _removeText(node->next);
    if(s.size()) {
        xmlNode *text = ::xmlNewText((xmlChar *) s.c_str());
        assert(text != 0);
        ::xmlAddNextSibling(node, text);
    }
}


static string
_collectText(xmlNode *node)
{
    string result;
    visitText_(node, [&](xmlNode *node) {
        result += (const char *) node->content;
    });
    return result;
}


// ---------------
// QName functions
// ---------------


string
QName::tostring() const
{
    if(ns_.empty()) {
        return tag_;
    }

    string qname(2 + ns_.size() + tag_.size(), '\0');
    int p = 0;
    qname[p++] = '{';
    qname.replace(p, ns_.size(), ns_);
    qname[p += ns_.size()] = '}';
    qname.replace(p + 1, tag_.size(), tag_);
    return qname;
}


QName::QName(const string &ns, const string &tag)
    : ns_(ns)
    , tag_(tag)
{
}


QName::QName(const QName &other)
    : ns_(other.ns_)
    , tag_(other.tag_)
{
}


QName::QName(const string &qname)
{
    if(qname.size() > 0 && qname[0] == '{') {
        size_t e = qname.find('}');
        if(e == string::npos) {
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


QName::QName(const char *qname)
    : QName(std::string(qname))
{}


const string &
QName::tag() const
{
    return tag_;
}


const string &
QName::ns() const
{
    return ns_;
}


bool
QName::equals(const char *ns, const char *tag) const
{
    if(ns) {
        if(ns_ != ns) {
            return false;
        }
    } else if(ns_.size()) {
        return false;
    }

    return tag_ == tag;
}


bool
QName::operator==(const QName &other) const
{
    return other.tag_ == tag_ && other.ns_ == ns_;
}


bool
QName::operator!=(const QName &other) const
{
    return !(other == *this);
}


// ----------------------
// XPathContext functions
// ----------------------


static std::vector<Element>
xpathNodesetToVector_(xmlNodeSet *set)
{
    std::vector<Element> out;
    for(int i = 0; set && i < set->nodeNr; i++) {
        xmlNode *node = set->nodeTab[i];
        if(node->type == XML_ELEMENT_NODE) {
            out.push_back(set->nodeTab[i]);
        }
    }
    return out;
}


XPathContext::~XPathContext()
{
    std::lock_guard<std::mutex> lock(mtx_);
    ::xmlXPathFreeContext(context_);
    context_ = NULL;
}


XPathContext::XPathContext(const etree::ns_list &ns_list)
{
    ::xmlResetLastError();

    context_ = xmlXPathNewContext(NULL);
    if(! context_) {
        maybeThrow_();
        throw internal_error();
    }

    for(auto &ns : ns_list) {
        auto prefix = toXmlChar_(ns.first.c_str());
        auto href = toXmlChar_(ns.second.c_str());
        int rc = ::xmlXPathRegisterNs(context_, prefix, href);
        if(rc) {
            ::xmlXPathFreeContext(context_);
            context_ = NULL;
            maybeThrow_();
            throw internal_error();
        }
    }
}


static ns_list
xpathContextToNsList_(const _xmlXPathContext *context_)
{
    ns_list ns_list;

    auto callback = [](void *payload, void *data, xmlChar *name) {
        auto &ns_list = *reinterpret_cast<::etree::ns_list *>(data);
        ns_list.emplace_back(reinterpret_cast<const char *>(name),
                             reinterpret_cast<const char *>(payload));
    };

    auto context = const_cast<_xmlXPathContext *>(context_);
    ::xmlHashScan(context->nsHash, callback, reinterpret_cast<void *>(&ns_list));

    return ns_list;
}


XPathContext::XPathContext(const XPathContext &other)
    : XPathContext(xpathContextToNsList_(other.context_))
{
}


// ---------------
// XPath functions
// ---------------


XPath::~XPath()
{
    ::xmlXPathFreeCompExpr(expr_);
}


XPath::XPath(const string &s)
    : context_(NULL)
{
    ::xmlResetLastError();
    expr_ = ::xmlXPathCompile(toXmlChar_(s.c_str()));
    maybeThrow_();
    s_ = s;
}


XPath::XPath(const char *s)
    : XPath(string(s))
{
}


XPath::XPath(const XPath &other)
    : XPath(other.s_)
{
}


XPath::XPath(const string &s, const XPathContext &context)
    : XPath(s)
{
    context_ = &context;
}


const std::string &
XPath::expr() const
{
    return s_;
}


XPath &
XPath::operator =(const XPath &other)
{
    ::xmlResetLastError();
    auto newExpr = ::xmlXPathCompile(toXmlChar_(other.s_.c_str()));
    maybeThrow_();

    ::xmlXPathFreeCompExpr(expr_);
    expr_ = newExpr;

    s_ = other.s_;
    return *this;
}


Nullable<Element>
XPath::find(const Element &e) const
{
    std::vector<Element> out = findall(e);
    if(out.empty()) {
        return Nullable<Element>();
    }
    return Nullable<Element>(out[0]);
}


std::string
XPath::findtext(const Element &e, const string &default_) const
{
    std::vector<Element> out = findall(e);
    if(out.empty()) {
        return default_;
    }
    return out[0].text();
}


std::vector<Element>
XPath::findall(const Element &e) const
{
    xmlNode *node = nodeFor__<xmlNode *>(e);
    xmlXPathObject *res;

    ::xmlResetLastError();
    if(context_) {
        auto context = const_cast<XPathContext *>(context_);
        std::lock_guard<std::mutex> lock(context->mtx_);
        context->context_->node = node;
        res = xmlXPathCompiledEval(expr_, context->context_);
    } else {
        xmlXPathContext *ctx = xmlXPathNewContext(node->doc);
        if(! ctx) {
            throw memory_error();
        }
        ctx->node = node;
        res = xmlXPathCompiledEval(expr_, ctx);
        ::xmlXPathFreeContext(ctx);
    }

    if(! res) {
        maybeThrow_();
    }
    auto out = xpathNodesetToVector_(res->nodesetval);
    //::xmlXPathDebugDumpObject(stdout, res, 0);
    ::xmlXPathFreeObject(res);
    return out;
}


std::vector<Element>
XPath::removeall(Element &e) const
{
    auto elems = findall(e);
    for(auto &elem : elems) {
        elem.remove();
    }
    return elems;
}


// -------------------
// Attribute functions
// -------------------


Attribute::Attribute(xmlAttr *attr)
    : attr_(attr)
{
}


string
Attribute::tag() const
{
    if(! attr_) {
        return "";
    }
    return (const char *)attr_->name;
}


string
Attribute::ns() const
{
    if(! attr_) {
        return "";
    }
    return attr_->ns ? (const char *)attr_->ns->href : "";
}


QName
Attribute::qname() const
{
    if(attr_) {
        return QName(ns(), tag());
    }
    return QName("", "");
}


string
Attribute::value() const
{
    if(! attr_) {
        return "";
    }
    string out;
    auto s = ::xmlNodeGetContent((_xmlNode *) attr_);
    if(s) {
        out = toChar_(s);
        ::xmlFree(s);
    }
    return out;
}


// -----------------
// AttrMap iterators
// -----------------


AttrIterator::AttrIterator(_xmlNode *node, _xmlAttr *attr)
    : node_(node)
    , attr_(attr)
    , next_(0)
{
    if(node_) {
        ref(node_);
    }
    if(attr_) {
        next_ = attr_->next;
    }
}

AttrIterator::AttrIterator()
    : node_(0)
    , attr_(0)
{
}


AttrIterator::~AttrIterator()
{
    if(node_) {
        unref(node_);
    }
}


bool
AttrIterator::operator ==(const AttrIterator& other)
{
    return attr_ == other.attr_;
}


bool
AttrIterator::operator !=(const AttrIterator& other)
{
    return attr_ != other.attr_;
}


const Attribute
AttrIterator::operator *()
{
    assert(attr_ != NULL);
    return Attribute(attr_);
}


AttrIterator &
AttrIterator::operator ++()
{
    attr_ = next_;
    if(attr_) {
        next_ = attr_->next;
    }
    return *this;
}


// -----------------
// AttrMap functions
// -----------------

AttrMap::AttrMap(xmlNode *node)
    : node_(ref(node))
{
}


AttrMap::~AttrMap()
{
    unref(node_);
}


AttrIterator
AttrMap::begin() const
{
    return AttrIterator(node_, node_->properties);
}


AttrIterator
AttrMap::end() const
{
    return AttrIterator();
}


bool
AttrMap::has(const QName &qname) const
{
    return ::xmlHasNsProp(node_, c_str(qname.tag()), c_str(qname.ns()));
}


string
AttrMap::get(const QName &qname,
             const string &default_) const
{
    string out(default_);
    xmlChar *s = ::xmlGetNsProp(node_, c_str(qname.tag()), c_str(qname.ns()));

    if(s) {
        out = toChar_(s);
        ::xmlFree(s);
    }
    return out;
}


void
AttrMap::set(const QName &qname, const string &s)
{
    ::xmlSetNsProp(node_,
        getNs_(node_, node_, qname.ns()), c_str(qname.tag()), c_str(s));
}


#ifdef ETREE_0X
void
AttrMap::set(kv_list attribs)
{
    for(auto &kv : attribs) {
        set(kv.first, kv.second);
    }
}
#endif


std::vector<QName>
AttrMap::keys() const
{
    std::vector<QName> names;
    for(xmlAttr *p = node_->properties; p; p = p->next) {
        const char *ns = p->ns ? (const char *)p->ns->href : "";
        names.push_back(QName(ns, (const char *)p->name));
    }
    return names;
}


bool
AttrMap::remove(const QName &qname)
{
    xmlAttr *p = xmlHasNsProp(node_, c_str(qname.tag()), c_str(qname.ns()));
    if(p) {
        int rc = xmlRemoveProp(p);
        assert(rc == 0);
        return true;
    }
    return false;
}


size_t
AttrMap::size() const
{
    size_t i = 0;
    for(xmlAttr *p = node_->properties; p; p = p->next) {
        i++;
    }
    return i;
}


// ---------------------
// ElementTree functions
// ---------------------


ElementTree::~ElementTree()
{
    unref(node_);
}


ElementTree::ElementTree(_xmlDoc *doc)
    : node_(ref(doc))
{
}


Element ElementTree::getroot() const
{
    xmlNode *cur = node_->children;
    bool ok = nextElement_(cur);
    assert(ok);
    return Element(cur);
}


bool
ElementTree::operator==(const ElementTree &other) const
{
    return node_ == other.node_;
}


bool
ElementTree::operator!=(const ElementTree &other) const
{
    return node_ != other.node_;
}


ElementTree &
ElementTree::operator=(const ElementTree &e)
{
    if(e != *this) {
        unref(node_);
        node_ = ref(e.node_);
    }
    return *this;
}


// -------------------------
// ChildIterator functions
// -------------------------


ChildIterator::ChildIterator()
    : elem_()
{
}


ChildIterator::ChildIterator(const Element &e)
    : elem_(e)
{
}


ChildIterator::ChildIterator(const ChildIterator &other)
    : elem_(other.elem_)
{
}


ChildIterator
ChildIterator::operator++()
{
    if(! elem_) {
        throw out_of_bounds_error();
    }
    elem_ = (*elem_).getnext();
    return *this;
}


ChildIterator
ChildIterator::operator++(int)
{
    ChildIterator tmp(*this);
    operator++();
    return tmp;
}


bool
ChildIterator::operator==(const ChildIterator &other) const
{
    return elem_ == other.elem_;
}


bool
ChildIterator::operator!=(const ChildIterator &other) const
{
    return !(elem_ == other.elem_);
}


Element &
ChildIterator::operator*()
{
    return *elem_;
}


// -----------------
// Element functions
// -----------------


Element::~Element()
{
    unref(node_);
}


Element::Element(const Element &e)
    : node_(ref(e.node_))
{
}


Element::Element(_xmlNode *node)
    : node_(ref(node))
{
}


static xmlNode *
nodeFromQname_(const QName &qname)
{
    xmlDoc *doc = ::xmlNewDoc(0);
    if(! doc) {
        throw memory_error();
    }

    xmlNode *node = ::xmlNewDocNode(doc, 0,
        toXmlChar_(qname.tag().c_str()), 0);
    if(! node) {
        ::xmlFreeDoc(doc);
        throw memory_error();
    }

    ::xmlDocSetRootElement(doc, node);
    ref(node);

    if(qname.ns().size()) {
        xmlNs *ns = ::xmlNewNs(node, (xmlChar *)qname.ns().c_str(), 0);
        if(ns == 0) {
            unref(node);
            throw memory_error();
        }
        ::xmlSetNs(node, ns);
    }

    return node;
}


Element::Element(const QName &qname)
    : node_(nodeFromQname_(qname))
{
}


#ifdef ETREE_0X
Element::Element(const QName &qname, kv_list attribs)
    : node_(nodeFromQname_(qname))
{
    try {
        attrsFromList_(*this, attribs);
    } catch(...) {
        unref(node_);
        throw;
    }
}
#endif


size_t
Element::size() const
{
    return ::xmlChildElementCount(node_);
}


void
Element::ensurens(const string &uri)
{
    getNs_(node_, node_, uri);
}


QName
Element::qname() const
{
    return QName(ns(), tag());
}


void
Element::qname(const QName &qname)
{
    ns(qname.ns());
    tag(qname.tag());
}


string
Element::tag() const
{
    return toChar_(node_->name);
}


void
Element::tag(const string &s)
{
    assert(! s.empty());
    ::xmlNodeSetName(node_, c_str(s));
}


string
Element::ns() const
{
    if(node_->ns) {
        return toChar_(node_->ns->href);
    }
    return "";
}


void
Element::ns(const string &ns)
{
    if(ns.empty()) {
        node_->ns = NULL;
    } else {
        node_->ns = getNs_(node_, node_, ns);
    }
}


AttrMap
Element::attrib() const
{
    return AttrMap(node_);
}


string
Element::get(const QName &qname, const string &default_) const
{
    return attrib().get(qname, default_);
}


Element
Element::operator[] (size_t i)
{
    xmlNode *cur = node_->children;
    for(;;) {
        if(! nextElement_(cur)) {
            throw out_of_bounds_error();
        }
        if(! i--) {
            return cur;
        }
        cur = cur->next;
    }
}


bool
Element::operator==(const Element &e) const
{
    return node_ == e.node_;
}


bool
Element::operator!=(const Element &e) const
{
    return node_ != e.node_;
}


Element &
Element::operator=(const Element &e)
{
    if(e != *this) {
        unref(node_);
        node_ = ref(e.node_);
    }
    return *this;
}


Nullable<Element>
Element::child() const
{
    xmlNode *p = node_->children;
    if(nextElement_(p)) {
        return Element(p);
    }
    return Nullable<Element>();
}


Nullable<Element>
Element::child(const QName &qn) const
{
    for(xmlNode *cur = node_->children; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE) {
            if(qn.equals(nsToChar_(cur->ns), toChar_(cur->name))) {
                return Element(cur);
            }
        }
    }
    return Nullable<Element>();
}


Element
Element::ensurechild(const QName &qn)
{
    auto maybe = child(qn);
    return maybe ? *maybe : SubElement(*this, qn);
}


std::vector<Element>
Element::children(const QName &qn) const
{
    std::vector<Element> out;
    for(xmlNode *cur = node_->children; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE) {
            if(qn.equals(nsToChar_(cur->ns), toChar_(cur->name))) {
                out.push_back(cur);
            }
        }
    }
    return out;
}


std::vector<Element>
Element::children() const
{
    std::vector<Element> out;
    for(auto child : *this) {
        out.push_back(child);
    }
    return out;
}


Element
Element::copy()
{
    xmlDoc *doc = ::xmlNewDoc(0);
    if(! doc) {
        throw memory_error();
    }

    xmlNode *newNode = ::xmlDocCopyNode(node_, doc, 1);
    if(! newNode) {
        ::xmlFreeDoc(doc);
        throw memory_error();
    }

    ::xmlDocSetRootElement(doc, newNode);
    return Element(newNode);
}


Nullable<Element>
Element::find(const XPath &expr) const
{
    return expr.find(*this);
}


string
Element::findtext(const XPath &expr, const string &default_) const
{
    return expr.findtext(*this, default_);
}


std::vector<Element>
Element::findall(const XPath &expr) const
{
    return expr.findall(*this);
}


void
Element::append(Element &e)
{
    if(e.ancestorOf(*this)) {
        throw cyclical_tree_error();
    }

    xmlDoc *sourceDoc = e.node_->doc;
    xmlNode *next = e.node_->next;

    ::xmlUnlinkNode(e.node_);
    ::xmlAddChild(node_, e.node_);
    moveTail_(next, e.node_);
    reparent_(e.node_);

    if(sourceDoc != node_->doc) {
        ref(node_->doc);
        unref(sourceDoc);
    }
}


void
Element::insert(size_t i, Element &e)
{
    if(e.ancestorOf(*this)) {
        throw cyclical_tree_error();
    }

    xmlNode *child = node_->children;
    while(i && nextElement_(child)) {
        i--;
        child = child->next;
    }

    xmlDoc *sourceDoc = e.node_->doc;
    xmlNode *next = e.node_->next;

    if(child) {
        ::xmlAddPrevSibling(child, e.node_);
    } else {
        ::xmlUnlinkNode(e.node_);
        ::xmlAddChild(node_, e.node_);
    }

    moveTail_(next, e.node_);
    reparent_(e.node_);

    if(sourceDoc != node_->doc) {
        ref(node_->doc);
        unref(sourceDoc);
    }
}


void
Element::remove(Element &e)
{
    if(e.node_->parent == node_) {
        e.remove();
    }
}


void
Element::remove()
{
    if(node_->parent == reinterpret_cast<xmlNode *>(node_->doc)) {
        return;
    }

    xmlDoc *doc = ::xmlNewDoc(0);
    if(! doc) {
        throw memory_error();
    }

    xmlDoc *sourceDoc = node_->doc;
    xmlNode *next = node_->next;
    ::xmlUnlinkNode(node_);
    ::xmlDocSetRootElement(doc, node_);
    moveTail_(next, node_);
    reparent_(node_);

    ref(doc);
    unref(sourceDoc);
}


void
Element::graft()
{
    if(node_->parent == reinterpret_cast<xmlNode *>(node_->doc)) {
        return;
    }

    xmlDoc *doc = ::xmlNewDoc(0);
    if(! doc) {
        throw memory_error();
    }

    xmlNode *lastChild = 0;
    for(xmlNode *cur = node_->children; cur; cur = cur->next) {
        cur->parent = node_->parent;
        reparent_(cur);
        lastChild = cur;
    }

    xmlNode *nodeNext;
    if(node_->children) {
        node_->children->prev = node_->prev;
        nodeNext = node_->children;
    } else {
        nodeNext = node_->next;
    }

    if(node_->prev) {
        node_->prev->next = nodeNext;
    } else {
        node_->parent->children = nodeNext;
    }

    if(lastChild) {
        lastChild->next = node_->next;
    }
    if(node_->next) {
        node_->next->prev = lastChild;
    }

    node_->parent = 0;
    node_->children = 0;
    node_->prev = 0;
    node_->next = 0;

    xmlDoc *sourceDoc = node_->doc;
    ::xmlDocSetRootElement(doc, node_);
    reparent_(node_);

    ref(doc);
    unref(sourceDoc);
}


bool
Element::ancestorOf(const Element &e) const
{
    xmlNode *parent = nodeFor__<xmlNode *>(*this);
    xmlNode *child = nodeFor__<xmlNode *>(e);

    for(; child; child = child->parent) {
        if(parent == child) {
            return true;
        }
    }
    return false;
}


Nullable<Element>
Element::getprev() const
{
    for(auto cur = node_->prev; cur; cur = cur->prev) {
       if(cur->type == XML_ELEMENT_NODE) {
            return Nullable<Element>(cur);
       }
    }
    return Nullable<Element>();
}


Nullable<Element>
Element::getnext() const
{
    for(auto cur = node_->next; cur; cur = cur->next) {
       if(cur->type == XML_ELEMENT_NODE) {
            return Element(cur);
       }
    }
    return Nullable<Element>();
}


Nullable<Element>
Element::getparent() const
{
    switch(node_->parent->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
        case XML_DOCB_DOCUMENT_NODE:
            return Nullable<Element>();
        default:
            return Nullable<Element>(node_->parent);
    }
}


ElementTree
Element::getroottree() const
{
    return ElementTree(node_->doc);
}


string
Element::text() const
{
    return _collectText(node_->children);
}


void
Element::text(const string &s)
{
    _setNodeText(node_, s);
}


string
Element::tail() const
{
    return _collectText(node_->next);
}


void
Element::tail(const string &s)
{
    _setTailText(node_, s);
}


ChildIterator
Element::begin() const
{
    xmlNode *cur = node_->children;
    if(nextElement_(cur)) {
        return ChildIterator(cur);
    }
    return ChildIterator();
}


ChildIterator
Element::end() const
{
    return ChildIterator();
}


// -------------------------
// tostring() implementation
// -------------------------


static int
writeCallback(void *ctx, const char *buffer, int len)
{
    string *s = static_cast<string *>(ctx);
    s->append(buffer, len);
    return len;
}


static int
closeCallback(void *ctx)
{
    return 0;
}


string
tostring(const Element &e)
{
    string out;
    xmlSaveCtxt *ctx = ::xmlSaveToIO(writeCallback, closeCallback,
        static_cast<void *>(&out), 0, 0);

    int ret = ::xmlSaveTree(ctx, nodeFor__<xmlNode *>(e));
    ::xmlSaveClose(ctx);
    if(ret == -1) {
        throw serialization_error();
    }
    return out;
}


string
tostring(const ElementTree &t)
{
    string out;
    xmlSaveCtxt *ctx = ::xmlSaveToIO(writeCallback, closeCallback,
        static_cast<void *>(&out), 0, 0);

    auto doc = nodeFor__<xmlDoc *>(t);
    int ret = ::xmlSaveTree(ctx, reinterpret_cast<xmlNode *>(doc));
    ::xmlSaveClose(ctx);
    if(ret == -1) {
        throw serialization_error();
    }
    return out;
}


// ----------------
// Helper functions
// ----------------

Element
SubElement(Element &parent, const QName &qname)
{
    auto parentNode = nodeFor__<xmlNode *>(parent);
    auto tagStr = qname.tag();
    auto nsCstr = toXmlChar_(tagStr.c_str());
    auto node = ::xmlNewDocNode(parentNode->doc, 0, nsCstr, 0);
    ::xmlAddChild(parentNode, node);

    if(qname.ns().size()) {
        node->ns = getNs_(node, node, qname.ns()); // exceptions
    }

    return Element(node);
}


#ifdef ETREE_0X
Element
SubElement(Element &parent, const QName &qname, kv_list attribs)
{
    Element elem = SubElement(parent, qname);
    attrsFromList_(elem, attribs);
    return elem;
}
#endif


// -------------------------------------
// fromstring() / parse() implementation
// -------------------------------------


struct StringBuf {
    const char *s;
    size_t remain;
    StringBuf(const char *s, size_t remain) : s(s), remain(remain) {}
};


static int
dummyClose_(void *ignored)
{
    return 0;
}


/*static*/ int
stringBufRead__(void *strm, char *buffer, int len)
{
    StringBuf &sb = *static_cast<StringBuf *>(strm);
    size_t cnt = std::min(size_t(len), sb.remain);
    ::memcpy(buffer, sb.s, cnt);
    sb.s += cnt;
    sb.remain -= cnt;
    return cnt;
}


/*static*/ int
istreamRead__(void *strm, char *buffer, int len)
{
    std::istream &is = *static_cast<std::istream *>(strm);

    is.read(buffer, len);
    if(is.fail() && !is.eof()) {
        return -1;
    }
    return is.gcount();
}


/*static*/ int
fdRead__(void *strm, char *buffer, int len)
{
    int &fd = *static_cast<int *>(strm);
    return ::read(fd, buffer, len);
}

typedef xmlDoc *(*ReadIOFunc)(xmlInputReadCallback, 
                              xmlInputCloseCallback,
                              void *,
                              const char *,
                              const char *,
                              int);

typedef int (*ReadCbFunc)(void *, char *, int);


template<ReadIOFunc readIoFunc,
         ReadCbFunc readCbFunc,
         int options=XML_PARSE_NODICT,
         typename T>
static ElementTree
parse_(T obj)
{
    ::xmlResetLastError();
    xmlDoc *doc = readIoFunc(readCbFunc, dummyClose_,
                             static_cast<void *>(obj), 0, 0, options);

    xmlNode *c;
    if(doc && (c = doc->children, nextElement_(c))) {
        return ElementTree(doc);
    }

    ::xmlFreeDoc(doc); // NULL ok.
    maybeThrow_();
    throw parse_error();
}


Element
fromstring(const char *s, size_t n)
{
    if(n == 0) {
        n = ::strlen(s);
    }
    StringBuf sb(s, n);
    ElementTree doc = parse_<::xmlReadIO, stringBufRead__>(&sb);
    return doc.getroot();
}


ElementTree
parse(std::istream &is)
{
    return parse_<::xmlReadIO, istreamRead__>(&is);
}


ElementTree
parse(const string &path)
{
    std::ifstream is(path.c_str(), std::ios_base::binary);
    return parse(is);
}


ElementTree
parse(int fd)
{
    return parse_<xmlReadIO, fdRead__>(&fd);
}


// ---------------------
// etree::html namespace
// ---------------------


namespace html {


static const int options = (0
    |HTML_PARSE_RECOVER
    |HTML_PARSE_NOERROR
    |HTML_PARSE_NOWARNING);


Element
fromstring(const char *s)
{
    StringBuf sb(s, ::strlen(s));
    ElementTree doc = parse_<htmlReadIO, stringBufRead__, options>(&sb);
    return doc.getroot();
}


Element
fromstring(const string &s)
{
    StringBuf sb(s.data(), s.size());
    ElementTree doc = parse_<htmlReadIO, stringBufRead__, options>(&sb);
    return doc.getroot();
}


ElementTree
parse(std::istream &is)
{
    return parse_<htmlReadIO, istreamRead__, options>(&is);
}


ElementTree
parse(const string &path)
{
    std::ifstream is(path.c_str(), std::ios_base::binary);
    return etree::html::parse(is);
}


ElementTree
parse(int fd)
{
    return parse_<htmlReadIO, fdRead__, options>(&fd);
}


} // namespace


// -----------------
// iostreams support
// -----------------


ostream &
operator<< (ostream &out, const ElementTree &tree)
{
    out << "<ElementTree at " << nodeFor__<xmlDoc *>(tree) << ">";
    return out;
}


ostream &
operator<< (ostream &out, const Element &elem)
{
    out << "<Element " << elem.qname().tostring() << " at ";
    out << nodeFor__<xmlNode *>(elem);
    out << " with " << elem.size() << " children>";
    return out;
}


ostream &
operator<< (ostream& out, const QName& qname)
{
    if(qname.ns().size()) {
        out << "{" << qname.ns() << "}";
    }
    out << qname.tag();
    return out;
}


} // namespace


namespace std {
    size_t hash<etree::QName>::operator()(const etree::QName &x) const
    {
        return hash<std::string>()(x.tag()) ^ (hash<std::string>()(x.ns()) << 1);
    }
} // namespace
