#ifndef ETREE_ELEMENT_H
#define ETREE_ELEMENT_H

/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>


#if __cplusplus >= 201103L
#   include <initializer_list>
#   include <utility>
#   define ETREE_0X
#   define ETREE_EXPLICIT explicit
#else
#   define ETREE_EXPLICIT
#endif


// libxml forwards.
struct _xmlAttr;
struct _xmlDoc;
struct _xmlNode;
struct _xmlNs;
struct _xmlXPathCompExpr;
struct _xmlXPathContext;


/**
 * ElementTree namespace; public classes and functions are defined here.
*/
namespace etree {

using std::string;
using std::vector;

class AttrMap;
class Element;
class ElementTree;
class QName;
class ChildIterator;
class XPath;
class XPathContext;

#ifdef ETREE_0X
typedef std::pair<string, string> kv_pair;
typedef std::pair<string, string> ns_pair;
typedef std::initializer_list<kv_pair> kv_list;
#endif


/**
 * Construct a new child element.
 *
 * @param parent        Parent element.
 * @param qname         New element name.
 * @returns             New element.
 */
Element SubElement(Element &parent, const QName &qname);


#ifdef ETREE_0X
/**
 * C++11: append a new child to an element and return it, taking the
 * child's attributes from an initializer list.
 *
 * @param parent        Parent element.
 * @param qname         New element name.
 * @param attribs       List of attribute name-value pairs from initialization
 *                      list.
 * @returns             New element.
 */
Element SubElement(Element &parent, const QName &qname, kv_list attribs);
#endif


/**
 * Parse an XML document from a character array and return a reference to its
 * root node.
 *
 * @param s
 *      XML document as a string.
 * @param n
 *      Number of bytes to consume. If zero, assumes s is NUL-terminated.
 * @returns
 *      Root Element.
 */
Element fromstring(const char *s, size_t n=0);

/**
 * Serialize an element. See ElementTree::tostring() for another variant.
 *
 * @param e             Element to serialize.
 * @returns             UTF-8 encoded string.
 */
string tostring(const Element &e);

/**
 * Serialize a tree. See ElementTree::tostring() for another variant.
 *
 * @param e             Element to serialize.
 * @returns             UTF-8 encoded string.
 */
string tostring(const ElementTree &e);

/**
 * Parse an XML document from a STL istream and return it.
 *
 * @param is            Input stream.
 * @returns             ElementTree instance.
 */
ElementTree parse(std::istream &is);

/**
 * Parse an XML document from the filesystem and return it.
 *
 * @param path          Path to file.
 * @returns             ElementTree instance.
 */
ElementTree parse(const string &path);

/**
 * Parse an XML document from a file descriptor and return it.
 *
 * @param fd            File descriptor number.
 * @returns             ElementTree instance.
 */
ElementTree parse(int fd);


/**
 * ElementTree HTML namespace; public classes and functions are defined here.
*/
namespace html {
    /**
     * Parse an HTML document from a character array and return a reference to
     * its root node.
     *
     * @param s             Document fragment as a string.
     * @returns             Root Element.
     */
    Element fromstring(const char *s);

    /**
     * Parse an HTML document from a STL string and return a reference to its
     * root node.
     *
     * @param s             Document fragment as a string.
     * @returns             Root Element.
     */
    Element fromstring(const std::string &s);

    /**
     * Serialize an HTML element. See ElementTree::tostring() for another
     * variant.
     *
     * @param e             Element to serialize.
     * @returns             UTF-8 encoded string.
     */
    std::string tostring(const Element &e);

    /**
     * Parse an HTML document from a STL istream and return it.
     *
     * @param is            Input stream.
     * @returns             ElementTree instance.
     */
    ElementTree parse(std::istream &is);

    /**
     * Parse an HTML document from the filesystem and return it.
     *
     * @param path          Path to file.
     * @returns             ElementTree instance.
     */
    ElementTree parse(const std::string &path);

    /**
     * Parse an HTML document from a file descriptor and return it.
     *
     * @param fd            File descriptor number.
     * @returns             ElementTree instance.
     */
    ElementTree parse(int fd);
} // namespace etree::html


/**
 * Generate a human-readable description of an element tree.
 *
 * @param out           ostream to write description to.
 * @param elem          Object to describe.
 * @returns             ostream.
 */
std::ostream &operator<< (std::ostream &out, const ElementTree &elem);

/**
 * Generate a human-readable description of an element.
 *
 * @param out           ostream to write description to.
 * @param elem          Object to describe.
 * @returns             ostream.
 */
std::ostream &operator<< (std::ostream &out, const Element &elem);

/**
 * Generate a human-readable description of a QName.
 *
 * @param out           ostream to write description to.
 * @param qname         Object to describe.
 * @returns             ostream.
 */
std::ostream &operator<< (std::ostream &out, const QName &qname);


/**
 * Lightweight wrapper to add nullable semantics to another type. This template
 * is presently only implemented for the Element type, attempting to
 * instantiate it with other types will fail.
 *
 * Nullable objects can be implicitly converted to boolean to test whether
 * their contents exists, and dereferenced to access the stored value, if any:
 *
 * \code
 *      Nullable<Element> maybe = parent.child("foo");
 *      if(maybe) {
 *          // Nullable contains a value. "*maybe" will dereference its value.
 *      } else {
 *          // Nullable is empty. "*maybe" will throw an exception.
 *      }
 * \endcode
 */
template<typename T>
class Nullable {
    /// Storage for contained type. This allows an unset nullable to truly have
    //no constructed value, and also allows the value type to lack a default
    //constructor.
    unsigned char val_[sizeof(T)];

    /// True if val_ contains a value.
    bool set_;

    public:
    /**
     * Construct an unset Nullable.
     */
    Nullable();

    /**
     * Construct a set Nullable containing a value.
     *
     * @param val       Value to copy.
     */
    Nullable(const T &val);

    /**
     * Copy the value from another Nullable.
     *
     * @param val       Value to copy.
     */
    Nullable(const Nullable<T> &val);

    #ifdef ETREE_0X
    /**
     * C++11: construct a set Nullable by moving a value.
     *
     * @param val       Value to copy.
     */
    Nullable(T &&val);
    #endif

    /**
     * Destroy the contained value if this Nullable is set.
     */
    ~Nullable();

    /**
     * Return true if both nullables are empty, or both nullables contain the
     * same value.
     */

    bool operator==(const Nullable<T> &other) const;

    /**
     * Return true if both nullables are empty, or both nullables contain the
     * same value.
     */
    bool operator==(const T &other) const;

    /**
     * Copy the instance and its contained value, if any.
     */
    Nullable<T> &operator=(const Nullable<T> &other);

    /**
     * Evaluate to true if this Nullable is set.
     */
    ETREE_EXPLICIT operator bool() const;

    /**
     * Return the contained value, or throw missing_value_error().
     */
    T &operator *();

    /*
     * Return the contained value, or throw missing_value_error().
     */
    T *operator ->();

    /**
     * Return the contained value, or throw missing_value_error().
     */
    const T &operator *() const;

    /**
     * Return the contained value, or throw missing_value_error().
     */
    const T *operator ->() const;
};

/**
 * Convenient alias for Nullable<Element> to avoid typing "etree::" twice in
 * scopes that do not import Nullable and Element.
 */
typedef Nullable<Element> NullableElement;


/**
 * Canonical representation for a name-namespace pair, without namespace
 * prefix, in James Clark's <a
 * href="http://www.jclark.com/xml/xmlns.htm">Universal Names</a> notation.
 *
 * A QName is essentially a canonical string in the form:
 *      <code>{http://namespace.uri/}tag-name</code>
 */
class QName {
    /// Namespace part.
    string ns_;

    /// Tag part.
    string tag_;

    public:
    /**
     * Create a QName from a namespace-tag pair.
     *
     * @param ns        Namespace.
     * @param tag       Tag.
     */
    QName(const string &ns, const string &tag);

    /**
     * Copy a QName.
     *
     * @param other     QName to copy.
     */
    QName(const QName &other);

    /**
     * Create a QName from Universal Name notation.
     *
     * @param qname     Universal name.
     */
    QName(const string &qname);

    /**
     * Create a QName from Universal Name notation.
     *
     * @param qname     Universal name.
     */
    QName(const char *qname);

    /**
     * Serialize a QName in Universal Name notation.
     *
     * @returns         QName in Universal Name notation.
     */
    string tostring() const;

    /**
     * Return the tag part of the QName.
     */
    const string &tag() const;

    /**
     * Return the namespace URI part of the QName.
     */
    const string &ns() const;

    /**
     * Return true if the (raw) strings match the QName's content.
     *
     * @param ns
     *      NULL, or the namespace URI.
     * @param tag
     *      The tag.
     */
    bool equals(const char *ns, const char *tag) const;

    /**
     * Compare this QName to another.
     *
     * @param other     Other QName.
     * @returns         True if equal.
     */
    bool operator==(const QName &other) const;

    /**
     * Compare this QName to another.
     *
     * @param other     Other QName.
     * @returns         False if equal.
     */
    bool operator!=(const QName &other) const;
};


/**
 * Represent a list of namespaces and their associated prefixes that should be
 * defined while executing an XPath expression.
 */
typedef std::vector<std::pair<std::string, std::string>> ns_list;


/**
 * Manages a set of registered XPath namespaces and extension functions. Note
 * that this object is internally synchronized across threads, consider copying
 * it in each thread.
 */
class XPathContext {
    _xmlXPathContext *context_;
    std::mutex mtx_;

    // For mutex().
    friend XPath;

    public:
    ~XPathContext();
    XPathContext(const etree::ns_list &ns_list = {});
    XPathContext(const XPathContext &other);
};


/**
 * Manages a compiled XPath expression.
 */
class XPath {
    /** The context to execute within, or NULL for no context. */
    const XPathContext *context_;

    /** The underlying compiled expression. */
    _xmlXPathCompExpr *expr_;

    /** String representation of the expression. */
    string s_;

    public:
    /**
     * Destroy the compiled expression.
     */
    ~XPath();

    /**
     * Compile an expression from a character array.
     *
     * @param s         XPath expression.
     */
    XPath(const char *s);

    /**
     * Compile an expression from a STL string.
     *
     * @param s         XPath expression.
     */
    XPath(const string &s);

    /**
     * Compile an expression from a STL string.
     *
     * @param s
     *      XPath expression.
     * @param context
     *      XPathContext object.
     */
    XPath(const string &s, const XPathContext &context);

    /**
     * Copy an expression.
     *
     * @param other     XPath expression to copy.
     */
    XPath(const XPath &other);

    /**
     * Return a string representation of the compiled expression.
     */
    const string &expr() const;

    /**
     * Replace this expression with another.
     *
     * @param other     XPath expression to assign.
     */
    XPath &operator =(const XPath &other);

    /**
     * Return the first matching Element, if any, matching the expression.
     *
     * @param e         Root element to search from.
     * @returns         Matching Element, if any.
     */
    Nullable<Element> find(const Element &e) const;

    /**
     * Return all Elements matching the expression.
     *
     * @param e         Root element to search from.
     * @returns         Matching Elements.
     */
    vector<Element> findall(const Element &e) const;

    /**
     * Like XPath::findall, except remove each discovered element before
     * returning it.
     */
    vector<Element> removeall(Element &expr) const;

    /**
     * Return the text part of the first matching element.
     *
     * @param e
     *      Root element to search from.
     * @param default_
     *      String to return if text is not found.
     * @returns
     *      Text part of the first matching element, or the empty string.
     */
    string findtext(const Element &e, const string &default_="") const;
};


/**
 * Proxy value type yielded by AttrIterator.
 */
class Attribute
{
    _xmlAttr *attr_;

    public:
    Attribute(_xmlAttr *attr);

    /**
     * Return the attributes's tag name.
     */
    string tag() const;

    /**
     * Return the attributes's namespace.
     */
    string ns() const;

    /**
     * Return the attributes's QName.
     */
    QName qname() const;

    /**
     * Return the attribute's value.
     */
    string value() const;
};


/**
 * Represents iteration position produced by AttrMap::begin() and
 * AttrMap::end().
 *
 * AttrIterator caches its next pointer, so that it is safe to delete an
 * attribute via Element.attrib().remove() during iteration.
 *
 * AttrIterator holds a reference to its parent Element, so that it is safe to
 * remove the element from its source document during iteration.
 */
class AttrIterator
{
    _xmlNode *node_;
    _xmlAttr *attr_;
    _xmlAttr *next_;

    public:
    ~AttrIterator();
    AttrIterator();
    AttrIterator(_xmlNode *elem, _xmlAttr *attr);

    bool operator ==(const AttrIterator &other);
    bool operator !=(const AttrIterator &other);

    /**
     * Yield an Attribute representing the attribute at this position.
     */
    const Attribute operator *();

    AttrIterator &operator++();
};


/**
 * Represents a mapping of an Element's attribute names to their values.
 */
class AttrMap
{
    _xmlNode *node_;

    public:
    ~AttrMap();
    AttrMap(_xmlNode *elem);

    /**
     * Produce an AttrIterator pointing to the first attribute.
     */
    AttrIterator begin() const;

    /**
     * Produce an AttrIterator pointing past the last attribute.
     */
    AttrIterator end() const;

    /**
     * Return true if the Element has the named attribute.
     */
    bool has(const QName &qname) const;

    /**
     * Return an attribute's value, or some default.
     *
     * @param qname
     *      Attribute QName.
     * @param default_
     *      String to return if attribute is not found.
     */
    string get(const QName &qname, const string &default_="") const;

    /**
     * Add or replace attribute's value.
     *
     * @param qname
     *      Attribute QName.
     * @param s
     *      New attribute value.
     */
    void set(const QName &qname, const string &s);

    #ifdef ETREE_0X
    /**
     * C++0x: set multiple attribute values in a single call.
     *
     * @param attribs
     *      std::initializer_list of (QName, value) pairs.
     */
    void set(kv_list attribs);
    #endif

    /**
     * Return the QNames of all attributes present on the Element.
     */
    vector<QName> keys() const;

    /**
     * Remove an attribute if it exists, returning true if deletion occured.
     */
    bool remove(const QName &qname);

    /**
     * Return the number of attributes the element has.
     */
    size_t size() const;
};


/**
 * Represents a reference to the root of an XML tree, the document itself. Note
 * that an element's documents are created and destroyed dynamically in
 * response to Element::insert(), Element::append() and Element::remove().
 */
class ElementTree
{
    template<typename P, typename T>
    friend P nodeFor__(const T &);

    _xmlDoc *node_;

    public:
    ~ElementTree();
    ElementTree();
    ElementTree(_xmlDoc *doc);
    Element getroot() const;

    /**
     * Return true if the identity of this element is equal to another element,
     * i.e. both refer to the same DOM node in the same document.
     */
    bool operator==(const ElementTree &other) const;

    /**
     * Return true if the identity of this element is equal to another element,
     * i.e. both refer to the same DOM node in the same document.
     */
    bool operator!=(const ElementTree &other) const;

    /**
     * Replace this <strong>element reference</strong> with an element.
     * Note the underlying DOM node is not modified, only the reference is
     * updated.
     */
    ElementTree &operator=(const ElementTree&);
};


/**
 * Represents a reference to a single XML element.
 */
class Element
{
    template<typename P, typename T>
    friend P nodeFor__(const T &);

    /// Reference to internal node implementation.
    _xmlNode *node_;

    /// Never defined.
    Element();

    public:
    /**
     * Destroy the reference to the element, deallocating the underlying tree
     * if this was the last reference.
     */
    ~Element();

    /**
     * Construct a new reference to an element.
     *
     * @param e     Element to copy reference to.
     */
    Element(const Element &e);

    /**
     * \internal
     * Construct a reference to a DOM node.
     *
     * @param node  Element to reference.
     */
    Element(_xmlNode *node);

    /**
     * Construct an element, creating a dummy ElementTree to contain it. Due to
     * the need to have a dummy document to contain orphaned elements, it is
     * better to use SubElement() when possible.
     *
     * @param qname
     *      New element name.
     */
    Element(const QName &qname);

    #ifdef ETREE_0X
    /**
     * C++11: construct an element, creating a dummy ElementTree to contain it.
     * Due to the need to have a dummy document to contain orphaned elements,
     * it is better to use SubElement() when possible.
     *
     * @param qname
     *      New element name.
     * @param attribs
     *      List of attribute name-value pairs from initialization list.
     */
    Element(const QName &qname, kv_list attribs);
    #endif

    /**
     * Ensure a namespace is in scope for this element, or is defined on this
     * element. Consider calling this if you're going to be defining lots of
     * subelements or attributes on this element belonging to the namespace.
     *
     * @param uri
     *      The namespace URI.
     */
    void ensurens(const string &uri);

    /**
     * Return the element's QName.
     */
    QName qname() const;

    /**
     * Set the element's QName.
     */
    void qname(const QName &qname);

    /**
     * Return the element's tag name.
     */
    string tag() const;

    /**
     * Set the element's tag name.
     */
    void tag(const string &tag);

    /**
     * Return the element's namespace URI, or the empty string if it has none.
     */
    string ns() const;

    /**
     * Set the element's namespace URI, or the empty string to remove any
     * existing namespace.
     */
    void ns(const string &ns);

    /**
     * Return the element's attribute dictionary.
     */
    AttrMap attrib() const;

    /**
     * Fetch the value of an attribute.
     *
     * @param   qname       Attribute name to fetch.
     * @param   default_    Default value if attribute is missing.
     * @returns             String attribute value, or the default.
     */
    string get(const QName &qname, const string &default_="") const;

    /**
     * Return the number of children this element has.
     */
    size_t size() const;

    /**
     * Return a child element. Valid indices are 0..size().
     */
    Element operator[] (size_t i);

    /**
     * Return true if the identity of this element is equal to another Element,
     * i.e. both refer to the same DOM node.
     */
    bool operator==(const Element &other) const;

    /**
     * Return false if the identity of this Element is equal to another
     * Element, i.e. both refer to the same DOM node.
     */
    bool operator!=(const Element &other) const;

    /**
     * Replace this <strong>element reference</strong> with an element.
     * Note the underlying DOM node is not modified, only the reference is
     * updated.
     */
    Element &operator=(const Element&);

    /**
     * Return the first, if any exist.
     *
     * @returns
     *      Element, if any, otherwise an empty nullable.
     */
    Nullable<Element> child() const;

    /**
     * Return the first child matching a name, if any exist.
     *
     * @param qn
     *      Name of the child to locate.
     * @returns
     *      Element, if any, otherwise an empty nullable.
     */
    Nullable<Element> child(const QName &qn) const;

    /**
     * Like child(), except appends the element if it was missing.
     *
     * @param qn
     *      Name of the child to create or locate.
     * @returns
     *      The element.
     */
    Element ensurechild(const QName &qn);

    /**
     * Return children matching a name.
     *
     * @param qn        Name of the children to locate.
     * @returns         Vector of elements.
     */
    vector<Element> children(const QName &qn) const;

    /**
     * Return all children.
     *
     * @returns         Vector of elements.
     */
    vector<Element> children() const;

    /**
     * Execute an XPath expression rooted on this element.
     *
     * @param expr      Expression to element.
     * @returns         Vector of matching elements.
     */
    Nullable<Element> find(const XPath &expr) const;

    /**
     * \copybrief XPath::findtext
     *
     * @param expr
     *      XPath expression to match.
     * @param default_
     *      String to return if text is not found.
     * @returns
     *      Text part of the first matching element, or the empty string.
     */
    string findtext(const XPath &expr, const string &default_="") const;

    /**
     * \copybrief XPath::findall
     *
     * @param expr      XPath expression to match.
     * @returns         Matching elements.
     */
    vector<Element> findall(const XPath &expr) const;

    /**
     * Like XPath::findall, except remove each discovered element before
     * returning it.
     */
    vector<Element> removeall(const XPath &expr);

    /**
     * Append an element to this element.
     *
     * This unlinks an element from its parent document and tree location,
     * relinks it to this element's document, and inserts it as the last child
     * of this element.
     *
     * \note
     *  All existing Element objects automatically update to the new location.
     *
     * @param e         Element to append as a child.
     */
    void append(Element &e);

    /**
     * Insert an element as a child of this element at a specific position.
     *
     * This unlinks an element from its parent document and tree location,
     * relinks it to this element's document, and inserts it as the specified
     * child of this element.
     *
     * \note
     *  All existing Element objects automatically update to the new location.
     *
     * @param i
     *      Index to insert element at (0..size()-1). If the index is greater
     *      than size(), behaves like append().
     * @param e
     *      Element to append as a child.
     */
    void insert(size_t i, Element &e);

    /**
     * Remove a child element.
     *
     * The child is moved to an empty document of its own. For this reason,
     * calling remove() followed by insert() or append() is suboptimal; you
     * should call insert() or append() directly.
     *
     * @param e     Element 
     */
    void remove(Element &e);

    /**
     * Remove *this* element from its parent, if any.
     *
     * The element is moved to an empty document of its own. For this reason,
     * calling remove() followed by insert() or append() is suboptimal; you
     * should call insert() or append() directly.
     */
    void remove();

    /**
     * Remove this element from its parent, moving any child nodes to the
     * element's old place in the DOM tree. You cannot graft the root node.
     *
     * \code{.cpp}
     *      auto elem = etree::fromstring(
     *          "<root>"
     *              "<tag1/> Hello"
     *              "<tag2>"
     *                  "<tag3/>"
     *              "</tag2> there"
     *          "</root>"
     *      );
     *      elem.child("tag2")->graft();
     *      assert(etree::tostring(elem) == (
     *          "<root>"
     *              "<tag1/> Hello"
     *              "<tag3/> there"
     *          "</root>"
     *      ));
     * \endcode
     */
    void graft();

    /**
     * Copy this element and all elements below it to a new document, returning
     * a reference to the newly copied element.
     */
    Element copy();

    /**
     * Return true if this is an ancestor of some element.
     */
    bool ancestorOf(const Element &e) const;

    /**
     * Return the next sibling element, if any.
     */
    Nullable<Element> getnext() const;

    /**
     * Return the parent element, if this element is not the document root.
     */
    Nullable<Element> getparent() const;

    /**
     * Return the previous sibling element, if any.
     */
    Nullable<Element> getprev() const;

    /**
     * Return the ElementTree this element belongs to.
     */
    ElementTree getroottree() const;

    /**
     * Return the element's text part, or the empty string. The text part is
     * any text or CDATA nodes contained immediately underneath the element in
     * the DOM tree, but before the first child element.
     *
     * \code{.xml}
     *      <who>person<child>123</child>known</who>me
     * \endcode
     *
     * In this example, <code>who</code>'s text part is the string "person".
     */
    string text() const;

    /**
     * Set the element's text part.
     *
     * @param s     New text part.
     */
    void text(const string &s);

    /**
     * Return the element's tail part, or the empty string. The tail part is
     * any text or CDATA nodes contained immediately following the element's
     * end tag, but before the next sibling.
     *
     * \code{.xml}
     *      <who>person<child>123</child>known</who>me
     * \endcode
     *
     * In this example, <code>who</code>'s tail part is the string "me", and
     * <code>child</code>'s tail part is the string "known".
     */
    string tail() const;

    /**
     * Set the element's tail part.
     *
     * @param s     New tail part.
     */
    void tail(const string &s);

    /**
     * Produce a ChildIterator pointing at the first child.
     */
    ChildIterator begin() const;

    /**
     * Produce a ChildIterator pointing past the final child.
     */
    ChildIterator end() const;
};


/**
 * Represents iteration position produced by Element::begin() and
 * Element::end().
 */
class ChildIterator
{
    Nullable<Element> elem_;

    public:
    ChildIterator();
    ChildIterator(const Element &);
    ChildIterator(const ChildIterator &);
    ChildIterator operator++(int);
    ChildIterator operator++();
    bool operator==(const ChildIterator &) const;
    bool operator!=(const ChildIterator &) const;

    /**
     * Yield an Element representing the child at this position.
     */
    Element &operator*();
};


/**
 * Depth-first visit an element and all of its subelements.
 *
 * @param elem
 *      Element to visit.
 * @param func
 *      Function called as (void)func(Element&);
 */
template<typename Function>
void
visit(Element elem, Function func)
{
    func(elem);
    for(auto &child : elem.children()) {
        visit(child, func);
    }
}


#define EXCEPTION(name)                                 \
    struct name : public std::runtime_error {           \
        name() : std::runtime_error("etree::"#name) {}  \
    };

EXCEPTION(cyclical_tree_error)
EXCEPTION(internal_error)
EXCEPTION(invalid_xpath_error)
EXCEPTION(memory_error)
EXCEPTION(missing_namespace_error)
EXCEPTION(missing_value_error)
EXCEPTION(out_of_bounds_error)
EXCEPTION(parse_error)
EXCEPTION(qname_error)
EXCEPTION(serialization_error)

#undef EXCEPTION


/**
 * Thrown to indicate libxml2 raised a parse error.
 */
struct xml_error : public std::runtime_error
{
    xml_error(const char *s)
        : std::runtime_error(s) {}
};


} // namespace


namespace std {
    template<>
    struct hash<etree::QName>
    {
        size_t operator()(const etree::QName &x) const;
    };
} // namespace


#endif
