
#include "element.hpp"


namespace etree {


/// -------------------------
/// Internal helper functions
/// -------------------------

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


/// ---------------------
/// ElementTree functions
/// ---------------------

ElementTree::~ElementTree()
{
    if(doc_) {
        proxy_->unref();
    }
}


ElementTree::ElementTree()
    : proxy_(0)
    , doc_(0)
{
}




/// -----------------
/// Element functions
/// -----------------

Element::~Element()
{
    if(node_) {
        proxy_->unref();
    }
}


Element::Element()
    : proxy_(0)
    , node_(0)
{
}


Element::Element(const Element &e)
    : node_(e.node_)
{
    proxy_ = node_ ? NodeProxy::ref_node(node_) : 0;
}


#if __cplusplus >= 201103L
Element::Element(Element &&e)
    : node_(e.node_)
    , proxy_(e.proxy_)
{
    e.node_ = 0;
    e.proxy_ = 0;
}
#endif


Element::Element(xmlNodePtr node)
    : node_(node)
{
    proxy_ = NodeProxy::ref_node(node_);
}


Element::Element(const UniversalName &un)
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


size_t Element::size() const
{
    return ::xmlChildElementCount(node_);
}


UniversalName Element::uname() const
{
    return UniversalName(ns(), tag());
}


const char *Element::tag() const
{
    return node_ ? (const char *) node_->name : "";
}


const char *Element::ns() const
{
    if(node_ && node_->nsDef) {
        return (const char *) node_->nsDef->href;
    }
    return "";
}


AttrMap Element::attrib() const
{
    return AttrMap(node_);
}


std::string Element::get(const UniversalName &un,
                         const std::string &default_) const
{
    return attrib().get(un, default_);
}


Element::operator bool() const
{
    return node_ != 0;
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
            throw element_error("operator[] out of bounds");
        }
    }
    return Element(cur);
}


bool Element::isIndirectParent(const Element &e)
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


void Element::append(Element &e)
{
    std::cout << "appending " << e.node_ << " to " << node_ << std::endl;
    if((! *this) || isIndirectParent(e)) {
        throw element_error("cannot append indirect parent to child");
    }
    ::xmlUnlinkNode(e.node_);
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


Element Element::getparent() const
{
    if(node_->parent && node_->parent->type != XML_DOCUMENT_NODE) {
        return Element(node_->parent);
    }
    return Element();
}


xmlNodePtr Element::_node() const
{
    return node_;
}


std::string Element::text() const
{
    return node_ ? _collectText(node_->children) : "";
}


void Element::text(const std::string &s)
{
    if(node_) {
        _setNodeText(node_, s);
    }
}


std::string Element::tail() const
{
    return node_ ? _collectText(node_->next) : "";
}


void Element::tail(const std::string &s)
{
    if(node_) {
        _setTailText(node_, s);
    }
}


/// ----------------
/// Helper functions
/// ----------------

Element SubElement(Element &parent, const UniversalName &uname)
{
    if(! parent) {
        throw element_error("cannot create sub-element on empty element.");
    }

    Element elem(uname);
    parent.append(elem);
    return elem;
}


Element fromstring(const std::string &s)
{
    xmlDocPtr doc = ::xmlReadMemory(s.data(), s.size(), 0, 0, 0);
    if(! doc) {
        return Element();
    }

    return Element(doc->children);
}


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
    std::string out;
    xmlSaveCtxtPtr ctx = ::xmlSaveToIO(writeCallback, closeCallback,
        static_cast<void *>(&out), 0, 0);

    int ret = ::xmlSaveTree(ctx, e._node());
    ::xmlSaveClose(ctx);
    if(ret == -1) {
        throw serialization_error();
    }

    return out;
}


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


} // namespace
