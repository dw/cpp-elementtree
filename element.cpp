
/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <cstdio> // snprintf().
#include <cstring>

#include <unistd.h>

#include <fstream>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "element.hpp"


namespace etree {

using std::string;
using std::ostream;


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

static xmlDocPtr ref(xmlDocPtr doc)
{
    // Relies on NULL (aka. initial state of _private) being (intptr_t)0, which
    // isn't true on some weird archs.
    assert(doc && (sizeof(void *) >= sizeof(intptr_t)));
    (*reinterpret_cast<intptr_t *>(&(doc->_private)))++;
    return doc;
}

static void unref(xmlDocPtr doc)
{
    assert(doc && (sizeof(void *) >= sizeof(intptr_t)));
    assert(doc->_private);
    if(! --*reinterpret_cast<intptr_t *>(&(doc->_private))) {
        xmlFreeDoc(doc);
    }
}

static xmlNodePtr ref(xmlNodePtr node)
{
    assert(node);
    if(! (*reinterpret_cast<intptr_t *>(&(node->_private)))++) {
        ref(node->doc);
    }
    return node;
}

static void unref(xmlNodePtr node)
{
    assert(node);
    assert(node->_private);
    if(! --*reinterpret_cast<intptr_t *>(&(node->_private))) {
        unref(node->doc);
    }
}


// -------------------------
// Internal helper functions
// -------------------------

const char *toChar_(const xmlChar *s)
{
    return reinterpret_cast<const char *>(s);
}

char *toChar_(xmlChar *s)
{
    return reinterpret_cast<char *>(s);
}

const xmlChar *toXmlChar_(const char *s)
{
    return reinterpret_cast<const xmlChar *>(s);
}

xmlChar *toXmlChar_(char *s)
{
    return reinterpret_cast<xmlChar *>(s);
}

static void maybeThrow_()
{
    xmlErrorPtr error = ::xmlGetLastError();
    if(error) {
        throw xml_error(error->message);
    }
}


#ifdef ETREE_0X
static void attrs_from_list_(Element &elem, kv_list &attribs)
{
    std::cout << "LOL!" << std::endl;
    AttrMap attrs = elem.attrib();
    for(auto &kv : attribs) {
        std::cout << "APPEND: " << kv.first << " V " << kv.second << "\n" << std::endl;
        attrs.set(kv.first, kv.second);
    }
}
#endif


static const xmlChar *c_str(const string &s)
{
    return toXmlChar_(s.empty() ? 0 : s.c_str());
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


static void _setNodeText(xmlNodePtr node, const string &s)
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


static void _setTailText(xmlNodePtr node, const string &s)
{
    _removeText(node->next);
    if(s.size()) {
        xmlNodePtr text = ::xmlNewText((xmlChar *) s.c_str());
        assert(text != 0);
        ::xmlAddNextSibling(node, text);
    }
}


static string _collectText(xmlNodePtr node)
{
    string result;
    for(;;) {
        node = _textNodeOrSkip(node);
        if(! node) {
            break;
        }
        result += (const char *) node->content;
        node = node->next;
    }
    return result;
}


static xmlNsPtr makeNs_(xmlNodePtr node, const string &uri)
{
    xmlChar prefix[6];
    xmlNsPtr ns;

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


static xmlNsPtr getNs_(xmlNodePtr node, xmlNodePtr target, const string &uri)
{
    if(uri.empty()) {
        return 0;
    }

    // Look for existing definition.
    xmlNodePtr doc_node = reinterpret_cast<xmlNodePtr>(node->doc);
    for(xmlNodePtr cur = node; cur && cur != doc_node; cur = cur->parent) {
        for(xmlNsPtr ns = node->nsDef; ns != 0; ns = ns->next) {
            if(uri == toChar_(ns->href)) {
                return ns;
            }
        }
    }

    return makeNs_(target, uri);
}


template<typename P, typename T>
P nodeFor__(const T &e)
{
    return e.node_;
}


// ---------------
// QName functions
// ---------------


void QName::fromString_(const string &qname)
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


string QName::tostring() const
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
    fromString_(qname);
}


QName::QName(const char *qname)
{
    fromString_(qname);
}


const string &QName::tag() const
{
    return tag_;
}


const string &QName::ns() const
{
    return ns_;
}


bool QName::operator==(const QName &other)
{
    return other.tag_ == tag_ && other.ns_ == ns_;
}


// ---------------
// XPath functions
// ---------------

static xmlXPathCompExprPtr compileOrThrow(const string &s)
{
    xmlXPathCompExprPtr expr = ::xmlXPathCompile(toXmlChar_(s.c_str()));
    maybeThrow_();
    return expr;
}


XPath::~XPath()
{
    ::xmlXPathFreeCompExpr(expr_);
}


XPath::XPath(const string &s)
{
    expr_ = compileOrThrow(s);
    s_ = s;
}


XPath::XPath(const char *s)
{
    expr_ = compileOrThrow(s);
    s_ = s;
}


XPath::XPath(const XPath &other)
{
    // No _private, can't refcount.
    expr_ = compileOrThrow(other.s_);
    s_ = other.s_;
}


const std::string &XPath::expr() const
{
    return s_;
}


XPath &XPath::operator =(const XPath &other)
{
    ::xmlXPathFreeCompExpr(expr_);
    expr_ = compileOrThrow(other.s_);
    s_ = other.s_;
    return *this;
}


Nullable<Element> XPath::find(const Element &e) const
{
    std::vector<Element> out = findall(e);
    if(out.empty()) {
        return Nullable<Element>();
    }
    return Nullable<Element>(out[0]);
}


std::string XPath::findtext(const Element &e) const
{
    std::vector<Element> out = findall(e);
    if(out.empty()) {
        return "";
    }
    return out[0].text();
}


std::vector<Element> XPath::findall(const Element &e) const
{
    std::vector<Element> out;
    xmlNodePtr node = nodeFor__<xmlNodePtr>(e);
    xmlXPathContextPtr ctx = xmlXPathNewContext(node->doc);
    if(! ctx) {
        throw memory_error();
    }

    ctx->node = node;
    xmlXPathObjectPtr res = xmlXPathCompiledEval(expr_, ctx);
    if(! res) {
        ::xmlXPathFreeContext(ctx);
        throw memory_error();
    }

    xmlNodeSetPtr set = res->nodesetval;
    if(set) {
        for(int i = 0; i < set->nodeNr; i++) {
            xmlNodePtr node = set->nodeTab[i];
            if(node->type == XML_ELEMENT_NODE) {
                out.push_back(Element(set->nodeTab[i]));
            }
        }
    }

    //::xmlXPathDebugDumpObject(stdout, res, 0);
    ::xmlXPathFreeObject(res);
    ::xmlXPathFreeContext(ctx);
    return out;
}


// -----------------
// AttrMap iterators
// -----------------

AttrIterator::AttrIterator(xmlNodePtr node)
    : node_(node)
{
}


QName AttrIterator::key()
{
    const char *ns = "";
    if(node_->nsDef) {
        ns = toChar_(node_->nsDef->href);
    }
    return QName(ns, toChar_(node_->name));
}


string AttrIterator::value()
{
    string out;
    xmlChar *s = ::xmlNodeListGetString(node_->doc,
                                        node_->children, 1);
    if(s) {
        out = toChar_(s);
        ::xmlFree(s);
    }

    return out;
}


bool AttrIterator::next()
{
    if(node_->next) {
        node_ = ref(node_->next);
        unref(node_->prev);
        return true;
    }
    return false;
}


// -----------------
// AttrMap functions
// -----------------

AttrMap::AttrMap(xmlNodePtr node)
    : node_(ref(node))
{
}


AttrMap::~AttrMap()
{
    unref(node_);
}


bool AttrMap::has(const QName &qname) const
{
    return ::xmlHasNsProp(node_, c_str(qname.tag()), c_str(qname.ns()));
}


string AttrMap::get(const QName &qname,
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


void AttrMap::set(const QName &qname, const string &s)
{
    ::xmlSetNsProp(node_,
        getNs_(node_, node_, qname.ns()), c_str(qname.tag()), c_str(s));
}


std::vector<QName> AttrMap::keys() const
{
    std::vector<QName> names;
    xmlAttrPtr p = node_->properties;
    while(p) {
        const char *ns = p->ns ? (const char *)p->ns->href : "";
        names.push_back(QName(ns, (const char *)p->name));
        p = p->next;
    }
    return names;
}


// ---------------------
// ElementTree functions
// ---------------------

ElementTree::~ElementTree()
{
    unref(node_);
}


ElementTree::ElementTree(xmlDocPtr doc)
    : node_(ref(doc))
{
}


Element ElementTree::getroot() const
{
    xmlNodePtr cur;
    for(cur = node_->children; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE) {
            return Element(cur);
        }
    }
    throw memory_error();
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


Element::Element(xmlNodePtr node)
    : node_(ref(node))
{
}


static xmlNodePtr node_from_qname(const QName &qname)
{
    xmlDocPtr doc = ::xmlNewDoc(0);
    if(! doc) {
        throw memory_error();
    }

    xmlNodePtr node = ::xmlNewDocNode(doc, 0,
        toXmlChar_(qname.tag().c_str()), 0);
    if(! node) {
        ::xmlFreeDoc(doc);
        throw memory_error();
    }

    ::xmlDocSetRootElement(doc, node);
    ref(node);

    if(qname.ns().size()) {
        xmlNsPtr ns = ::xmlNewNs(node, (xmlChar *)qname.ns().c_str(), 0);
        if(ns == 0) {
            unref(node);
            throw memory_error();
        }
        ::xmlSetNs(node, ns);
    }

    return node;
}


Element::Element(const QName &qname)
    : node_(node_from_qname(qname))
{
}


#ifdef ETREE_0X
Element::Element(const QName &qname, kv_list attribs)
    : node_(node_from_qname(qname))
{
    try {
        attrs_from_list_(*this, attribs);
    } catch(...) {
        unref(node_);
        throw;
    }
}
#endif


size_t Element::size() const
{
    return ::xmlChildElementCount(node_);
}


QName Element::qname() const
{
    return QName(ns(), tag());
}


void Element::qname(const QName &qname)
{
    ns(qname.ns());
    tag(qname.tag());
}


string Element::tag() const
{
    return toChar_(node_->name);
}


void Element::tag(const string &s)
{
    assert(! s.empty());
    ::xmlNodeSetName(node_, c_str(s));
}


string Element::ns() const
{
    if(node_->ns) {
        return toChar_(node_->ns->href);
    }
    return "";
}


void Element::ns(const string &ns)
{
    if(ns.empty()) {
        node_->ns = NULL;
    } else {
        node_->ns = getNs_(node_, node_, ns);
    }
}


AttrMap Element::attrib() const
{
    return AttrMap(node_);
}


string Element::get(const QName &qname, const string &default_) const
{
    return attrib().get(qname, default_);
}


Element Element::operator[] (size_t i)
{
    xmlNodePtr cur = node_->children;
    while(i) {
        if(cur->type == XML_ELEMENT_NODE) {
            i--;
        }
        cur = cur->next;
        if(! cur) {
            throw out_of_bounds_error();
        }
    }
    return Element(cur);
}


bool Element::operator==(const Element &e)
{
    return node_ == e.node_;
}


Element &Element::operator=(const Element &e)
{
    if(this != &e) {
        unref(node_);
        node_ = ref(e.node_);
    }
    return *this;
}


Nullable<Element> Element::child(const QName &qn) const
{
    for(xmlNodePtr cur = node_->children; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE) {
            Element elem = Element(cur);
            if(elem.qname() == qn) {
                return elem;
            }
        }
    }
    return Nullable<Element>();
}


std::vector<Element> Element::children(const QName &qn) const
{
    std::vector<Element> out;
    for(xmlNodePtr cur = node_->children; cur; cur = cur->next) {
        if(cur->type == XML_ELEMENT_NODE) {
            Element elem = Element(cur);
            if(elem.qname() == qn) {
                out.push_back(elem);
            }
        }
    }
    return out;
}


Nullable<Element> Element::find(const XPath &expr) const
{
    return expr.find(*this);
}


string Element::findtext(const XPath &expr) const
{
    return expr.findtext(*this);
}


std::vector<Element> Element::findall(const XPath &expr) const
{
    return expr.findall(*this);
}


bool Element::isIndirectParent(const Element &e)
{
    xmlNodePtr other = e.node_;
    xmlNodePtr parent = node_->parent;
    while(parent) {
        if(parent == other) {
            return true;
        }
        parent = parent->parent;
    }
    return false;
}


void Element::append(Element &e)
{
    std::cout << "appending " << e;
    std::cout << " to " << *this << std::endl;
    if(isIndirectParent(e)) {
        throw cyclical_tree_error();
    }
    ::xmlUnlinkNode(e.node_);
    unref(e.node_->doc);
    e.node_->doc = ref(node_->doc);
    ::xmlAddChild(node_, e.node_);
}


void Element::insert(size_t i, Element &e)
{
    if(i == size()) {
        append(e);
    } else {
        ::xmlAddPrevSibling(this[i].node_, e.node_);
    }
}


void Element::remove(Element &e)
{
    if(e.node_->parent == node_) {
        ::xmlUnlinkNode(e.node_);
    }
}


Nullable<Element> Element::getnext() const
{
    if(node_->next) {
        return Nullable<Element>(ref(node_->next));
    }
    return Nullable<Element>();
}


Nullable<Element> Element::getparent() const
{
    xmlNodePtr parent = node_->parent;
    if(parent && parent->type != XML_DOCUMENT_NODE) {
        return Nullable<Element>(ref(parent));
    }
    return Nullable<Element>();
}


ElementTree Element::getroottree() const
{
    return ElementTree(node_->doc);
}


string Element::text() const
{
    return _collectText(node_->children);
}


void Element::text(const string &s)
{
    _setNodeText(node_, s);
}


string Element::tail() const
{
    return _collectText(node_->next);
}


void Element::tail(const string &s)
{
    _setTailText(node_, s);
}


// -------------------------
// tostring() implementation
// -------------------------


static int writeCallback(void *ctx, const char *buffer, int len)
{
    string *s = static_cast<string *>(ctx);
    s->append(buffer, len);
    return len;
}


static int closeCallback(void *ctx)
{
    return 0;
}


string tostring(const Element &e)
{
    string out;
    xmlSaveCtxtPtr ctx = ::xmlSaveToIO(writeCallback, closeCallback,
        static_cast<void *>(&out), 0, 0);

    int ret = ::xmlSaveTree(ctx, nodeFor__<xmlNodePtr>(e));
    ::xmlSaveClose(ctx);
    if(ret == -1) {
        throw serialization_error();
    }
    return out;
}


// ----------------
// Helper functions
// ----------------

Element SubElement(Element &parent, const QName &qname)
{
    Element elem(qname);
    parent.append(elem);
    return elem;
}


#ifdef ETREE_0X
Element SubElement(Element &parent, const QName &qname, kv_list attribs)
{
    Element elem = SubElement(parent, qname);
    attrs_from_list_(elem, attribs);
    return elem;
}
#endif


static Element fromstring_internal(const char *s, size_t size)
{
    xmlDocPtr doc = ::xmlReadMemory(s, size, 0, 0, 0);
    if(! (doc && doc->children)) {
        ::xmlFreeDoc(doc); // NULL ok.
        maybeThrow_();
        throw parse_error();
    }

    return ElementTree(doc).getroot();
}


Element fromstring(const char *s)
{
    return fromstring_internal(s, ::strlen(s));
}


Element fromstring(const string &s)
{
    return fromstring_internal(s.data(), s.size());
}


// ----------------------
// parse() implementation
// ----------------------

static int dummyClose_(void *ignored)
{
    return 0;
}


int istreamRead__(void *strm, char *buffer, int len)
{
    std::istream &is = *static_cast<std::istream *>(strm);

    is.read(buffer, len);
    if(is.fail() && !is.eof()) {
        return -1;
    }
    return is.gcount();
}


int fdRead__(void *strm, char *buffer, int len)
{
    int &fd = *static_cast<int *>(strm);
    return ::read(fd, buffer, len);
}


template<int(*fn)(void *, char *, int), typename T>
static ElementTree parseInternal_(T obj)
{
    xmlDocPtr doc = ::xmlReadIO(fn, dummyClose_,
                                static_cast<void *>(obj), 0, 0, 0);
    if(! doc) {
        maybeThrow_();
        throw parse_error();
    }
    return ElementTree(doc);
}


ElementTree parse(std::istream &is)
{
    return parseInternal_<istreamRead__>(&is);
}


ElementTree parse(const string &path)
{
    std::ifstream is(path.c_str(), std::ios_base::binary);
    return parse(is);
}


ElementTree parse(int fd)
{
    return parseInternal_<fdRead__>(&fd);
}


// -----------------
// iostreams support
// -----------------


ostream &operator<< (ostream &out, const ElementTree &tree)
{
    out << "<ElementTree at " << nodeFor__<xmlDocPtr>(tree) << ">";
    return out;
}


ostream &operator<< (ostream &out, const Element &elem)
{
    out << "<Element " << elem.qname().tostring() << " at ";
    out << nodeFor__<xmlNodePtr>(elem);
    out << " with " << elem.size() << " children>";
    return out;
}


ostream& operator<< (ostream& out, const QName& qname)
{
    if(qname.ns().size()) {
        out << "{" << qname.ns() << "}";
    }
    out << qname.tag();
    return out;
}


} // namespace