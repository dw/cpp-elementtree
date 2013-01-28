/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <iostream>

#include "feed.hpp"

using namespace std;

namespace etree {
namespace feed {

using namespace etree;

using std::string;

typedef const std::vector<QName> NameList;


// ----------------
// Helper functions
// ----------------

static Element getChild_(Element &parent, const QName &qn)
{
    Nullable<Element> maybe = parent.child(qn);
    return maybe ? *maybe : SubElement(parent, qn);
}


static string getText_(const Element cur, const NameList &names)
{
    for(int i = 0; i < names.size(); i++) {
        Nullable<Element> maybe = cur.child(names[i]);
        if(! maybe) {
            return "";
        }
        const_cast<Element &>(cur) = *maybe;
    }
    return cur.text();
}


static void setText_(Element cur, const NameList &names, const string &s)
{
    for(int i = 0; i < names.size(); i++) {
        cur = getChild_(cur, names[i]);
    }
    cur.text(s);
}


static std::vector<Item> itemsFromPath_(const ItemFormat &format,
    const Element &elem, const NameList &names)
{
    std::vector<Item> out;

    Element cur = const_cast<Element&>(elem);
    for(int i = 0; i < (names.size() - 1); i++) {
        Nullable<Element> maybe = cur.child(names[i]);
        if(! maybe) {
            return out;
        }
        cur = *maybe;
    }

    for(auto &elem : cur.children(names.back())) {
        out.push_back(Item(format, elem));
    }
    return out;
}


class FeedFormat
{
    public:
    virtual bool identify(const Element &e) const = 0;
    virtual enum feed_format format() const = 0;
    virtual string title(const Element &) const = 0;
    virtual void title(Element &, const string &) const = 0;
    virtual string link(const Element &) const = 0;
    virtual void link(Element &, const string &) const = 0;
    virtual string description(const Element &) const = 0;
    virtual void description(Element &, const string &) const = 0;
    virtual string icon(const Element &) const = 0;
    virtual void icon(Element &, const string &) const = 0;
    virtual std::vector<Item> items(const Element &) const = 0;
};


class ItemFormat
{
    public:
    virtual string title(const Element &) const = 0;
    virtual void title(Element &, const string &) const = 0;
    virtual string link(const Element &) const = 0;
    virtual void link(Element &, const string &) const = 0;
    virtual string content(const Element &) const = 0;
    virtual void content(Element &, const string &) const = 0;
    virtual enum content_type type(const Element &) const = 0;
    virtual void type(Element &, enum content_type) const = 0;
    virtual string author(const Element &) const = 0;
    virtual void author(Element &, const string &) const = 0;
    virtual string guid(const Element &) const = 0;
    virtual void guid(Element &, const string &) const = 0;
    virtual string originalGuid(const Element &) const = 0;
    virtual time_t published(const Element &) const = 0;
    virtual void published(Element &, time_t) const = 0;
};


// -------------------
// Item implementation
// -------------------

Item::Item(const ItemFormat &format, const Element &elem)
    : format_(format), elem_(elem) {}

Item::Item(const Item &other)
    : format_(other.format_)
    , elem_(other.elem_) {}

string Item::title() const              { return format_.title(elem_); }
void Item::title(const string &s)       { format_.title(elem_, s); }
string Item::link() const               { return format_.link(elem_); }
void Item::link(const string &s)        { format_.link(elem_, s); }
string Item::content() const            { return format_.content(elem_); }
void Item::content(const string &s)     { format_.content(elem_, s); }
content_type Item::type() const         { return format_.type(elem_); }
void Item::type(content_type type)      { format_.type(elem_, type); }
string Item::author() const             { return format_.author(elem_); }
void Item::author(const string &s)      { format_.author(elem_, s); }
string Item::guid() const               { return format_.guid(elem_); }
void Item::guid(const string &s)        { format_.guid(elem_, s); }
string Item::originalGuid() const       { return format_.originalGuid(elem_); }
time_t Item::published() const          { return format_.published(elem_); }
void Item::published(time_t published)  { format_.published(elem_, published); }


// -------------------
// Feed implementation
// -------------------

Feed::Feed(const FeedFormat &format, const Element &elem)
    : format_(format), elem_(elem) {}

Feed::Feed(const Feed &other)
    : format_(other.format_)
    , elem_(other.elem_) {}

enum feed_format Feed::format() const   { return format_.format(); }
string Feed::title() const              { return format_.title(elem_); }
void Feed::title(const string &s)       { format_.title(elem_, s); }
string Feed::link() const               { return format_.link(elem_); }
void Feed::link(const string &s)        { format_.link(elem_, s); }
string Feed::description() const        { return format_.description(elem_); }
void Feed::description(const string &s) { format_.description(elem_, s); }
std::vector<Item> Feed::items() const   { return format_.items(elem_); }
Element Feed::element() const           { return elem_; }


// -------------------
// Atom implementation
// -------------------

#define READER_NS "{http://www.google.com/schemas/reader/atom/}"
#define ATOM_NS "{http://www.w3.org/2005/Atom}"

static const QName kAtomContentTag{ATOM_NS "content"};
static const QName kAtomSummaryTag{ATOM_NS "summary"};
static const QName kAtomLinkTag = ATOM_NS "link";
static const QName kAtomRootTag = ATOM_NS "feed";
static const QName kAtomRelAttr = "rel";
static const QName kAtomTypeAttr = "type";
static const QName kAtomOriginalGuidAttr = READER_NS "original-id";
static const NameList kAtomSummaryPath{kAtomSummaryTag};
static const NameList kAtomContentPath{kAtomContentTag};
static const NameList kAtomAuthorPath{ATOM_NS "author", ATOM_NS "name"};
static const NameList kAtomGuidPath{ATOM_NS "id"};
static const NameList kAtomItemsPath{ATOM_NS "entry"};
static const NameList kAtomLinkPath{kAtomLinkTag};
static const NameList kAtomPublishedPath{ATOM_NS "published"};
static const NameList kAtomTitlePath{ATOM_NS "title"};


class AtomItemFormat
    : public ItemFormat
{
    public:
    static AtomItemFormat instance;

    string title(const Element &e) const
    {
        return getText_(e, kAtomTitlePath);
    }

    void title(Element &e, const string &s) const
    {
        setText_(e, kAtomTitlePath, s);
    }

    string link(const Element &e) const
    {
        for(auto &elem : e.children(kAtomLinkTag)) {
            if(elem.get(kAtomRelAttr) == "alternate"
                && elem.get(kAtomTypeAttr) == "text/html") {
                return elem.get("href");
            }
        }
        return "";
    }

    void link(Element &e, const string &s) const
    {
        setText_(e, kAtomLinkPath, s);
    }

    Nullable<Element> getContentTag_(const Element &e) const
    {
        static const std::vector<const QName *> tags = {
            &kAtomContentTag,
            &kAtomSummaryTag
        };

        Nullable<Element> out;
        for(auto &tag : tags) {
            out = e.child(*tag);
            if(out) {
                break;
            }
        }
        return out;
    }

    string content(const Element &e) const
    {
        Nullable<Element> content = getContentTag_(e);
        return content ? (*content).text() : "";
    }

    void content(Element &e, const string &s) const
    {
        Nullable<Element> content;
        while((content = getContentTag_(e))) {
            (*content).remove();
        }
        setText_(e, kAtomContentPath, s);
    }

    enum content_type type(const Element &e) const
    {
        Nullable<Element> content = e.child(kAtomContentTag);
        if(content) {
            string type = (*content).get(kAtomTypeAttr);
            if(type == "html" || type == "xhtml") {
                return CTYPE_HTML;
            }
        }
        return CTYPE_TEXT;
    }

    void type(Element &e, enum content_type type) const
    {
        const char *s = (type == CTYPE_HTML) ? "html" : "text";
        getChild_(e, kAtomContentTag).attrib().set(kAtomTypeAttr, s);
    }

    string author(const Element &e) const
    {
        return getText_(e, kAtomAuthorPath);
    }

    void author(Element &e, const string &s) const
    {
        setText_(e, kAtomAuthorPath, s);
    }

    string guid(const Element &e) const
    {
        return getText_(e, kAtomGuidPath);
    }

    void guid(Element &e, const string &s) const
    {
        setText_(e, kAtomGuidPath, s);
    }

    string originalGuid(const Element &e) const
    {
        Nullable<Element> idElem = e.child(ATOM_NS "id");
        if(idElem) {
            string out = (*idElem).get(kAtomOriginalGuidAttr);
            if(out.size()) {
                return out;
            }
        }
        return guid(e);
    }

    time_t published(const Element &e) const
    {
        return parseIso8601Date_(getText_(e, kAtomPublishedPath));
    }

    void published(Element &e, time_t) const
    {
        assert(0);
    }
};


struct AtomFeedFormat
    : public FeedFormat
{
    public:
    static AtomFeedFormat instance;

    bool identify(const Element &e) const
    {
        return e.qname() == kAtomRootTag;
    }

    enum feed_format format() const
    {
        return FORMAT_ATOM;
    }

    string title(const Element &e) const
    {
        return getText_(e, kAtomTitlePath);
    }

    void title(Element &e, const string &s) const
    {
        setText_(e, kAtomTitlePath, s);
    }

    string link(const Element &e) const
    {
        return getText_(e, kAtomLinkPath);
    }

    void link(Element &e, const string &s) const
    {
        setText_(e, kAtomLinkPath, s);
    }

    string icon(const Element &e) const
    {
        return "";
    }

    void icon(Element &e, const string &s) const
    {
    }

    string description(const Element &e) const
    {
        return getText_(e, kAtomContentPath);
    }

    void description(Element &e, const string &s) const
    {
        setText_(e, kAtomContentPath, s);
    }

    std::vector<Item> items(const Element &e) const {
        return itemsFromPath_(AtomItemFormat::instance, e, kAtomItemsPath);
    }
};


// ------------------------
// Rss20Feed implementation
// ------------------------

static const NameList kRssContentPath{"channel", "description"};
static const NameList kRssItemPublishedPath{"pubDate"};
static const NameList kRssItemDescrPath{"description"};
static const NameList kRssItemGuidPath{"guid"};
static const NameList kRssItemLinkPath{"link"};
static const NameList kRssItemTitlePath{"title"};
static const NameList kRssItemsPath{"channel", "item"};
static const NameList kRssLinkPath{"channel", "link"};
static const NameList kRssTitlePath{"channel", "title"};


class Rss20ItemFormat
    : public ItemFormat
{
    public:
    static Rss20ItemFormat instance;

    string title(const Element &e) const
    {
        return getText_(e, kRssTitlePath);
    }

    void title(Element &e, const string &s) const
    {
        setText_(e, kRssTitlePath, s);
    }

    string link(const Element &e) const
    {
        return getText_(e, kRssLinkPath);
    }

    void link(Element &e, const string &s) const
    {
        setText_(e, kRssLinkPath, s);
    }

    string content(const Element &e) const
    {
        return getText_(e, kRssContentPath);
    }

    void content(Element &e, const string &s) const
    {
        setText_(e, kRssContentPath, s);
    }

    enum content_type type(const Element &e) const
    {
        return CTYPE_HTML;
    }

    void type(Element &e, enum content_type type) const
    {
        assert(type == CTYPE_HTML);
    }

    string author(const Element &e) const
    {
        return "";
    }

    void author(Element &e, const string &s) const
    {
    }

    string guid(const Element &e) const
    {
        return getText_(e, kRssItemGuidPath);
    }

    void guid(Element &e, const string &s) const
    {
        setText_(e, kRssItemGuidPath, s);
    }

    string originalGuid(const Element &e) const
    {
        return guid(e);
    }

    time_t published(const Element &e) const
    {
        return parseRfc822Date_(getText_(e, kRssItemPublishedPath));
    }

    void published(Element &e, time_t) const
    {
        assert(0);
    }
};


class Rss20FeedFormat
    : public FeedFormat
{
    public:
    static Rss20FeedFormat instance;

    bool identify(const Element &e) const
    {
        return e.tag() == "rss" && e.get("version", "2.0") == "2.0";
    }

    enum feed_format format() const
    {
        return FORMAT_RSS20;
    }

    string title(const Element &e) const
    {
        return getText_(e, kRssTitlePath);
    }

    void title(Element &e, const string &s) const
    {
        setText_(e, kRssTitlePath, s);
    }

    string link(const Element &e) const
    {
        return getText_(e, kRssLinkPath);
    }

    void link(Element &e, const string &s) const
    {
        setText_(e, kRssLinkPath, s);
    }

    string description(const Element &e) const
    {
        return getText_(e, kRssContentPath);
    }

    void description(Element &e, const string &s) const
    {
        setText_(e, kRssContentPath, s);
    }

    string icon(const Element &e) const
    {
        return "";
    }

    void icon(Element &e, const string &s) const
    {
    }

    time_t published(const Element &e) const
    {
        return parseRfc822Date_(getText_(e, kRssItemPublishedPath));
    }

    void published(Element &e, time_t) const
    {
        assert(0);
    }

    std::vector<Item> items(const Element &e) const {
        return itemsFromPath_(Rss20ItemFormat::instance, e, kRssItemsPath);
    }
};


AtomItemFormat AtomItemFormat::instance;
Rss20ItemFormat Rss20ItemFormat::instance;
AtomFeedFormat AtomFeedFormat::instance;
Rss20FeedFormat Rss20FeedFormat::instance;
static const std::vector<const FeedFormat *> formats_ = {
    &AtomFeedFormat::instance,
    &Rss20FeedFormat::instance
};



Feed fromelement(Element elem)
{
    for(auto &format : formats_) {
        if(format->identify(elem)) {
            return Feed(*format, elem);
        }
    }
    throw memory_error();
}


} // namespace
} // namespace
