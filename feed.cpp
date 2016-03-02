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

typedef const std::vector<QName> NameList;


// ----------------
// Helper functions
// ----------------


template<typename T>
const FeedFormat &formatFor__(const T &item)
{
    return item.format_;
}

static Element getChild_(Element &parent, const QName &qn)
{
    Nullable<Element> maybe = parent.child(qn);
    return maybe ? *maybe : SubElement(parent, qn);
}


static std::string getText_(const Element cur, const NameList &names)
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


static void setText_(Element cur, const NameList &names, const std::string &s)
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
    virtual ItemFormat *item_format() const = 0;
    virtual std::string title(const Element &) const = 0;
    virtual void title(Element &, const std::string &) const = 0;
    virtual std::string link(const Element &) const = 0;
    virtual void link(Element &, const std::string &) const = 0;
    virtual std::string description(const Element &) const = 0;
    virtual void description(Element &, const std::string &) const = 0;
    virtual std::string icon(const Element &) const = 0;
    virtual void icon(Element &, const std::string &) const = 0;
    virtual std::vector<Item> items(const Element &) const = 0;
    virtual Feed create() const = 0;
    virtual Item append(Element &) const = 0;
};


class ItemFormat
{
    public:
    virtual std::string title(const Element &) const = 0;
    virtual void title(Element &, const std::string &) const = 0;
    virtual std::string link(const Element &) const = 0;
    virtual void link(Element &, const std::string &) const = 0;
    virtual std::string content(const Element &) const = 0;
    virtual void content(Element &, const std::string &) const = 0;
    virtual enum content_type type(const Element &) const = 0;
    virtual void type(Element &, enum content_type) const = 0;
    virtual std::string author(const Element &) const = 0;
    virtual void author(Element &, const std::string &) const = 0;
    virtual std::string guid(const Element &) const = 0;
    virtual void guid(Element &, const std::string &) const = 0;
    virtual std::string originalGuid(const Element &) const = 0;
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

std::string Item::title() const              { return format_.title(elem_); }
void Item::title(const std::string &s)       { format_.title(elem_, s); }
std::string Item::link() const               { return format_.link(elem_); }
void Item::link(const std::string &s)        { format_.link(elem_, s); }
std::string Item::content() const            { return format_.content(elem_); }
void Item::content(const std::string &s)     { format_.content(elem_, s); }
content_type Item::type() const         { return format_.type(elem_); }
void Item::type(content_type type)      { format_.type(elem_, type); }
std::string Item::author() const             { return format_.author(elem_); }
void Item::author(const std::string &s)      { format_.author(elem_, s); }
std::string Item::guid() const               { return format_.guid(elem_); }
void Item::guid(const std::string &s)        { format_.guid(elem_, s); }
std::string Item::originalGuid() const       { return format_.originalGuid(elem_); }
time_t Item::published() const          { return format_.published(elem_); }
void Item::published(time_t published)  { format_.published(elem_, published); }
Element Item::element() const           { return elem_; }

void Item::remove() {
    elem_.remove();
}


// -------------------
// Feed implementation
// -------------------

Feed::Feed(const FeedFormat &format, const Element &elem)
    : format_(format), elem_(elem) {}

Feed::Feed(const Feed &other)
    : format_(other.format_)
    , elem_(other.elem_) {}

enum feed_format Feed::format() const   { return format_.format(); }
std::string Feed::title() const              { return format_.title(elem_); }
void Feed::title(const std::string &s)       { format_.title(elem_, s); }
std::string Feed::link() const               { return format_.link(elem_); }
void Feed::link(const std::string &s)        { format_.link(elem_, s); }
std::string Feed::description() const        { return format_.description(elem_); }
void Feed::description(const std::string &s) { format_.description(elem_, s); }
std::string Feed::icon() const        { return format_.icon(elem_); }
void Feed::icon(const std::string &s) { format_.icon(elem_, s); }
std::vector<Item> Feed::items() const   { return format_.items(elem_); }
Element Feed::element() const           { return elem_; }

Item Feed::append() {
    auto item = format_.append(elem_);
    item.title("");
    item.link("");
    item.link("");
    item.type(CTYPE_HTML);
    item.author("");
    item.guid("");
    item.published(0);
    return item;
}

void Feed::append(Item item) {
}


// -------------------
// Atom implementation
// -------------------

#define READER_NS "{http://www.google.com/schemas/reader/atom/}"
#define ATOM_NS "http://www.w3.org/2005/Atom"

static const XPathContext kAtomContext = XPathContext(ns_list{
    {"atom", ATOM_NS}
});

static const XPath kAtomLinkPath = XPath(
    "atom:link[@rel='alternate' and @type='text/html']",
    kAtomContext);
static const XPath kAtomIconPath = XPath(
    "atom:icon | atom:image",
    kAtomContext);
static const XPath kAtomEntryPath = XPath(
    "atom:entry",
    kAtomContext);
static const XPath kAtomTitlePath = XPath(
    "atom:title",
    kAtomContext);
static const XPath kAtomSubtitlePath = XPath(
    "atom:subtitle",
    kAtomContext);

static const QName kAtomFeedTag(ATOM_NS, "feed");
static const QName kAtomEntryTag(ATOM_NS, "entry");
static const QName kAtomContentTag(ATOM_NS, "content");
static const QName kAtomSummaryTag(ATOM_NS, "summary");
static const QName kAtomTitleTag(ATOM_NS, "title");
static const QName kAtomSubtitleTag(ATOM_NS, "subtitle");
static const QName kAtomLinkTag(ATOM_NS, "link");
static const QName kAtomRootTag(ATOM_NS, "feed");
static const QName kAtomTypeAttr = "type";
static const QName kAtomOriginalGuidAttr = READER_NS "original-id";
static const NameList kAtomSummaryPath{kAtomSummaryTag};
static const NameList kAtomContentPath{kAtomContentTag};
static const NameList kAtomAuthorPath({{ATOM_NS, "author"},
                                       {ATOM_NS, "name"}});
static const NameList kAtomGuidPath{{ATOM_NS, "id"}};
static const NameList kAtomPublishedPath{{ATOM_NS, "published"}};


class AtomItemFormat
    : public ItemFormat
{
    public:
    static AtomItemFormat instance;

    std::string title(const Element &e) const
    {
        return kAtomTitlePath.findtext(e);
    }

    void title(Element &e, const std::string &s) const
    {
        kAtomTitlePath.removeall(e);
        auto title = SubElement(e, kAtomTitleTag, {
            {"type", "text"}
        });
        title.text(s);
    }

    std::string link(const Element &e) const
    {
        auto elem = kAtomLinkPath.find(e);
        if(elem) {
            return elem->attrib().get("href");
        }
        return "";
    }

    void link(Element &e, const std::string &s) const
    {
        kAtomLinkPath.removeall(e);
        etree::SubElement(e, kAtomLinkTag, {
            {"rel", "alternate"},
            {"type", "text/html"},
            {"href", s}
        });
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

    std::string content(const Element &e) const
    {
        Nullable<Element> content = getContentTag_(e);
        return content ? (*content).text() : "";
    }

    void content(Element &e, const std::string &s) const
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
            std::string type = (*content).get(kAtomTypeAttr);
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

    std::string author(const Element &e) const
    {
        return getText_(e, kAtomAuthorPath);
    }

    void author(Element &e, const std::string &s) const
    {
        setText_(e, kAtomAuthorPath, s);
    }

    std::string guid(const Element &e) const
    {
        return getText_(e, kAtomGuidPath);
    }

    void guid(Element &e, const std::string &s) const
    {
        setText_(e, kAtomGuidPath, s);
    }

    std::string originalGuid(const Element &e) const
    {
        Nullable<Element> idElem = e.child({ATOM_NS, "id"});
        if(idElem) {
            std::string out = (*idElem).get(kAtomOriginalGuidAttr);
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
        //assert(0);
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

    std::string title(const Element &e) const
    {
        return kAtomTitlePath.findtext(e);
    }

    void title(Element &e, const std::string &s) const
    {
        kAtomTitlePath.removeall(e);
        auto title = SubElement(e, kAtomTitleTag, {
            {"type", "text"}
        });
        title.text(s);
    }

    std::string link(const Element &e) const
    {
        auto elem = kAtomLinkPath.find(e);
        if(elem) {
            return elem->attrib().get("href");
        }
        return "";
    }

    void link(Element &e, const std::string &s) const
    {
        kAtomLinkPath.removeall(e);
        etree::SubElement(e, kAtomLinkTag, {
            {"rel", "alternate"},
            {"type", "text/html"},
            {"href", s}
        });
    }

    std::string icon(const Element &e) const
    {
        return kAtomIconPath.findtext(e);
    }

    void icon(Element &e, const std::string &s) const
    {
    }

    std::string description(const Element &e) const
    {
        return kAtomSubtitlePath.findtext(e);
    }

    void description(Element &e, const std::string &s) const
    {
        kAtomSubtitlePath.removeall(e);
        auto subtitle = SubElement(e, kAtomSubtitleTag, {
            {"type", "text"}
        });
        subtitle.text(s);
    }

    ItemFormat *item_format() const
    {
        return &AtomItemFormat::instance;
    }

    std::vector<Item> items(const Element &e) const
    {
        std::vector<Item> out;
        for(auto &elem : kAtomEntryPath.findall(e)) {
            out.push_back(Item(AtomItemFormat::instance, elem));
        }
        return out;
    }

    Feed create() const
    {
        return Feed(AtomFeedFormat::instance,
            Element(kAtomFeedTag));
    }

    Item append(Element &e) const
    {
        return Item(AtomItemFormat::instance,
            SubElement(e, kAtomEntryTag));
    }
};


// ------------------------
// Rss20Feed implementation
// ------------------------
//
static const XPath kRssLinkPath{"channel/link"};
static const XPath kRssIconPath{"channel/image/url"};
static const XPath kRssTitlePath{"channel/title"};
static const XPath kRssItemLinkPath{"title"};
static const XPath kRssItemTitlePath{"title"};

static const NameList kRssContentPath{"channel", "description"};
static const NameList kRssItemPublishedPath{"pubDate"};
static const NameList kRssItemDescrPath{"description"};
static const NameList kRssItemGuidPath{"guid"};
static const NameList kRssItemsPath{"channel", "item"};


static Element
getRssChannel_(Element &e)
{
    auto maybe = e.child("channel");
    if(maybe) {
        return *maybe;
    }
    return SubElement(e, "channel");
}


class Rss20ItemFormat
    : public ItemFormat
{
    public:
    static Rss20ItemFormat instance;

    std::string title(const Element &e) const
    {
        return kRssItemTitlePath.findtext(e);
    }

    void title(Element &e, const std::string &s) const
    {
        kRssItemTitlePath.removeall(e);
        SubElement(e, "title").text(s);
    }

    std::string link(const Element &e) const
    {
        return kRssItemLinkPath.findtext(e);
    }

    void link(Element &e, const std::string &s) const
    {
        kRssItemLinkPath.removeall(e);
        SubElement(e, "link").text(s);
    }

    std::string content(const Element &e) const
    {
        return getText_(e, kRssContentPath);
    }

    void content(Element &e, const std::string &s) const
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

    std::string author(const Element &e) const
    {
        return "";
    }

    void author(Element &e, const std::string &s) const
    {
    }

    std::string guid(const Element &e) const
    {
        return getText_(e, kRssItemGuidPath);
    }

    void guid(Element &e, const std::string &s) const
    {
        setText_(e, kRssItemGuidPath, s);
    }

    std::string originalGuid(const Element &e) const
    {
        return guid(e);
    }

    time_t published(const Element &e) const
    {
        return parseRfc822Date_(getText_(e, kRssItemPublishedPath));
    }

    void published(Element &e, time_t) const
    {
        //assert(0);
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

    std::string title(const Element &e) const
    {
        return kRssTitlePath.findtext(e);
    }

    void title(Element &e, const std::string &s) const
    {
        kRssTitlePath.removeall(e);
        auto channel = getRssChannel_(e);
        SubElement(channel, "title").text(s);
    }

    std::string link(const Element &e) const
    {
        return kRssLinkPath.findtext(e);
    }

    void link(Element &e, const std::string &s) const
    {
        kRssLinkPath.removeall(e);
        auto channel = getRssChannel_(e);
        SubElement(channel, "link").text(s);
    }

    std::string description(const Element &e) const
    {
        return getText_(e, kRssContentPath);
    }

    void description(Element &e, const std::string &s) const
    {
        setText_(e, kRssContentPath, s);
    }

    std::string icon(const Element &e) const
    {
        return kRssIconPath.findtext(e);
    }

    void icon(Element &e, const std::string &s) const
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

    ItemFormat *item_format() const
    {
        return &Rss20ItemFormat::instance;
    }

    std::vector<Item> items(const Element &e) const
    {
        return itemsFromPath_(Rss20ItemFormat::instance, e, kRssItemsPath);
    }

    Feed create() const
    {
        return Feed(Rss20FeedFormat::instance,
            Element("rss"));
    }

    Item append(Element &e) const
    {
        return Item(Rss20ItemFormat::instance,
            SubElement(e, kAtomEntryTag));
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




static const FeedFormat *formatFromEnum_(enum feed_format n)
{
    for(auto &format : formats_) {
        if(format->format() == n) {
            return format;
        }
    }
    assert(0);
    return NULL;
}


Feed
create(enum feed_format f)
{
    auto format = formatFromEnum_(f);
    auto feed = format->create();
    return feed;
}


Feed fromelement(Element elem)
{
    for(auto &format : formats_) {
        if(format->identify(elem)) {
            return Feed(*format, elem);
        }
    }
    throw memory_error();
}


Item itemFromElement(Element elem, enum feed_format format)
{
    ItemFormat *itemFormat = formatFromEnum_(format)->item_format();
    return Item(*itemFormat, elem);
}


} // namespace
} // namespace
