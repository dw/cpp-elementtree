
#include <algorithm>
#include <cstring>
#include <vector>
#include <cassert>
#include <stdexcept>
#include <utility>

#include "uname.hpp"


namespace etree {


class Element;
class ElementSet;
void maybeTakeNode(ElementSet *set, xmlNodePtr node);
Element SubElement(Element &parent, UniversalName uname);


struct element_error : public std::runtime_error
{
    element_error(const char *s) : std::runtime_error(s) {}
};


class AttrMap
{
    friend Element;

    xmlNodePtr node_;
    AttrMap(xmlNodePtr node) : node_(node) {}

    static const xmlChar *c_str(std::string s)
    {
        return (const xmlChar *) (s.empty() ? nullptr : s.c_str());
    }

    public:
    bool has(UniversalName un) const
    {
        return ::xmlHasNsProp(node_, c_str(un.tag()), c_str(un.ns()));
    }

    std::string get(UniversalName un, std::string default_="") const
    {
        const char *s = (const char *) xmlGetNsProp(node_, c_str(un.tag()),
                                                           c_str(un.ns()));
        if(s) {
            default_ = s;
            ::xmlFree((void *) s);
        }
        return default_;
    }

    const xmlNsPtr findNs_(std::string ns)
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

    void set(const UniversalName un, const std::string s)
    {
        ::xmlSetNsProp(node_, findNs_(un.ns()), c_str(un.tag()), c_str(s));
    }

    std::vector<UniversalName> keys() const
    {
        std::vector<UniversalName> names;
        xmlAttrPtr p = node_->properties;
        while(p) {
            const char *ns = p->ns ? (const char *) p->ns->href : "";
            const char *s = (const char *) ::xmlNodeGetContent((xmlNodePtr) p);
            assert(s != nullptr);
            names.push_back(UniversalName(ns, (const char *)p->name));
            p = p->next;
        }
        return names;
    }

    /*
    void operator[] (const UniversalName un, const std::string s)
    {
        ::xmlSetNsProp(node_, c_str(un.ns()), c_str(un.tag()), c_str(s));
    }
    */
};


class Element
{
    ElementSet *set_;
    xmlNodePtr node_;

    public:
    ~Element()
    {
        if(set_ && node_) {
            maybeTakeNode(set_, node_);
        }
    }

    Element() : set_(nullptr), node_(nullptr) {}
    Element(const Element &e) : set_(e.set_), node_(e.node_) {}
    Element(ElementSet *set, xmlNodePtr node) : set_(set), node_(node) {}
    Element(ElementSet *set, UniversalName un) : set_(set)
    {
        node_ = ::xmlNewNode(nullptr, (const xmlChar *)(un.tag().c_str()));
        if(node_ == nullptr) {
            throw element_error("allocaion failed");
        }

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

    ElementSet *set() const
    {
        return set_;
    }

    size_t size() const
    {
        return ::xmlChildElementCount(node_);
    }

    UniversalName uname() const
    {
        UniversalName un(ns(), tag());
        return un;
    }

    std::string tag() const
    {
        return node_ ? (const char *) node_->name : "";
    }

    std::string ns() const
    {
        if(node_ && node_->nsDef) {
            return (const char *) node_->nsDef->href;
        }
        return "";
    }

    AttrMap attrib() const
    {
        return AttrMap(node_);
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
        return Element(set_, cur);
    }

    bool isIndirectParent(Element &e)
    {
        if(node_) {
            xmlNodePtr parent = node_->parent;
            while(parent) {
                if(parent == e.node_) {
                    return true;
                }
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

    Element getparent()
    {
        Element e;
        if(node_->parent) {
            e = Element(set_, node_->parent);
        }
        return e;
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

    void _setNodeText(xmlNodePtr node, std::string &s)
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

    void _setTailText(xmlNodePtr node, std::string &s)
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

    void text(std::string s)
    {
        if(node_) {
            _setNodeText(node_, s);
        }
    }

    std::string tail() const
    {
        return node_ ? _collectText(node_->next) : "";
    }

    void tail(std::string s)
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


class ElementSet {
    friend void maybeTakeNode(ElementSet *set, xmlNodePtr node);
    std::vector<xmlNodePtr> nodes;

    void maybeTake(xmlNodePtr node)
    {
        if(node->parent == nullptr) {
            if(! std::binary_search(nodes.begin(), nodes.end(), node,
                                    std::greater<xmlNodePtr>())) {
                nodes.push_back(node);
                std::push_heap(nodes.begin(), nodes.end());
            }
        }
    }

    ElementSet(const ElementSet &) {}

    public:
    ElementSet() : nodes() {}

    ~ElementSet()
    {
        for(auto node : nodes) {
            if(! node->parent) {
                std::cout << "node with no parent: " << node << std::endl;
                xmlFreeNode(node);
            }
        }
    }

    Element element(UniversalName un)
    {
        Element e(this, un);
        return e;
    }

    Element fromstring(std::string s)
    {
        xmlDocPtr doc = ::xmlReadMemory(s.data(), s.size(),
            nullptr, nullptr, 0);
        assert(doc != nullptr);
    }
};


void maybeTakeNode(ElementSet *set, xmlNodePtr node)
{
    set->maybeTake(node);
}


Element SubElement(Element &parent, UniversalName &uname)
{
    ElementSet *set = parent.set();
    if(set == nullptr) {
        throw element_error("cannot create sub-element on empty element.");
    }

    Element elem = set->element(uname);
    parent.append(elem);
    return elem;
}


} // namespace
