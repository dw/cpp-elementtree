/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <cassert>
#include <iostream>

#include <elementtree/feed.hpp>


namespace etree {
namespace feed {


//
// Constants.
//


#define READER_NS "http://www.google.com/schemas/reader/atom/"
#define DUBLIN_CORE_NS "http://purl.org/dc/elements/1.1/"
#define ATOM_NS "http://www.w3.org/2005/Atom"

static const XPathContext kContext(ns_list{
    {"atom", ATOM_NS},
    {"dc", DUBLIN_CORE_NS}
});

#define ATOM_LINK_PATH "atom:link[@rel='alternate' and @type='text/html']"

static const QName kAtomAuthorTag(ATOM_NS, "author");
static const QName kAtomContentTag(ATOM_NS, "content");
static const QName kAtomEntryTag(ATOM_NS, "entry");
static const QName kAtomFeedTag(ATOM_NS, "feed");
static const QName kAtomIconTag(ATOM_NS, "icon");
static const QName kAtomIdTag(ATOM_NS, "id");
static const QName kAtomLinkTag(ATOM_NS, "link");
static const QName kAtomNameTag(ATOM_NS, "name");
static const QName kAtomOriginalGuidAttr(READER_NS, "original-id");
static const QName kAtomPublishedTag(ATOM_NS, "published");
static const QName kAtomRootTag(ATOM_NS, "feed");
static const QName kAtomSubtitleTag(ATOM_NS, "subtitle");
static const QName kAtomSummaryTag(ATOM_NS, "summary");
static const QName kAtomTitleTag(ATOM_NS, "title");
static const QName kAtomUpdatedTag(ATOM_NS, "updated");
static const XPath kAtomAuthorPath("atom:author/atom:name", kContext);
static const XPath kAtomContentPath("atom:content", kContext);
static const XPath kAtomEntryPath("atom:entry", kContext);
static const XPath kAtomGuidPath("atom:id", kContext);
static const XPath kAtomIconPath("atom:icon | atom:image", kContext);
static const XPath kAtomLinkPath(ATOM_LINK_PATH, kContext);
static const XPath kAtomPublishedPath("atom:published", kContext);
static const XPath kAtomSubtitlePath("atom:subtitle", kContext);
static const XPath kAtomTitlePath("atom:title", kContext);
static const XPath kAtomUpdatedPath("atom:updated", kContext);
static const XPath kDublinCoreCreatorPath("dc:creator", kContext);
static const XPath kRssIconPath("channel/image/url");
static const XPath kRssItemContentPath("description");
static const XPath kRssItemGuidPath("guid");
static const XPath kRssItemsPath("channel/item");
static const XPath kRssLinkPath("link");
static const XPath kRssPublishedPath("pubDate");
static const XPath kRssTitlePath("title");


// ----------------
// Helper functions
// ----------------


template<typename T>
const FeedFormat &formatFor__(const T &item)
{
    return item.format_;
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
    virtual time_t updated(const Element &) const = 0;
    virtual void updated(Element &, time_t) const = 0;
};


// -------------------
// Item implementation
// -------------------


Item::Item(const ItemFormat &format, const Element &elem)
    : format_(format)
    , elem_(elem)
{}


Item::Item(const Item &other)
    : format_(other.format_)
    , elem_(other.elem_)
{}


std::string
Item::title() const {
    return stripWs_(format_.title(elem_));
}


void
Item::title(const std::string &s) {
    format_.title(elem_, s);
}


std::string
Item::link() const {
    return stripWs_(format_.link(elem_));
}


void Item::link(const std::string &s) {
    format_.link(elem_, s);
}


std::string
Item::content() const {
    return stripWs_(format_.content(elem_));
}


void
Item::content(const std::string &s) {
    format_.content(elem_, s);
}


content_type Item::type() const {
    return format_.type(elem_);
}


void
Item::type(content_type type) {
    format_.type(elem_, type);
}


std::string
Item::author() const {
    return stripWs_(format_.author(elem_));
}


void
Item::author(const std::string &s) {
    format_.author(elem_, s);
}


std::string
Item::guid() const {
    return stripWs_(format_.guid(elem_));
}


void
Item::guid(const std::string &s) {
    format_.guid(elem_, s);
}


std::string
Item::originalGuid() const {
    return stripWs_(format_.originalGuid(elem_));
}


time_t
Item::published() const {
    return format_.published(elem_);
}


void
Item::published(time_t published) {
    format_.published(elem_, published);
}


time_t
Item::updated() const {
    return format_.updated(elem_);
}


void
Item::updated(time_t updated) {
    format_.updated(elem_, updated);
}


Element
Item::element() const {
    return elem_;
}


void
Item::remove() {
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
        auto title = e.ensurechild(kAtomTitleTag);
        title.attrib().set("type", "text");
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
        auto link = e.ensurechild(kAtomLinkTag);
        link.attrib().set({
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
        auto content = e.ensurechild(kAtomContentTag);
        content.text(s);
    }

    enum content_type type(const Element &e) const
    {
        Nullable<Element> content = e.child(kAtomContentTag);
        if(content) {
            std::string type = (*content).get("type");
            if(type == "html" || type == "xhtml") {
                return CTYPE_HTML;
            }
        }
        return CTYPE_TEXT;
    }

    void type(Element &e, enum content_type type) const
    {
        const char *s = (type == CTYPE_HTML) ? "html" : "text";
        e.ensurechild(kAtomContentTag).attrib().set("type", s);
    }

    std::string author(const Element &e) const
    {
        return kAtomAuthorPath.findtext(e);
    }

    void author(Element &e, const std::string &s) const
    {
        auto author = e.ensurechild(kAtomAuthorTag);
        auto name = author.ensurechild(kAtomNameTag);
        name.text(s);
    }

    std::string guid(const Element &e) const
    {
        return kAtomGuidPath.findtext(e);
    }

    void guid(Element &e, const std::string &s) const
    {
        e.ensurechild(kAtomIdTag).text(s);
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
        return parseIso8601Date_(kAtomPublishedPath.findtext(e));
    }

    void published(Element &e, time_t t) const
    {
        auto s = formatIso8601_(t);
        e.ensurechild(kAtomPublishedTag).text(s);
        if(! updated(e)) {
            updated(e, t);
        }
    }

    time_t updated(const Element &e) const
    {
        return parseIso8601Date_(kAtomUpdatedPath.findtext(e));
    }

    void updated(Element &e, time_t t) const
    {
        auto s = formatIso8601_(t);
        e.ensurechild(kAtomUpdatedTag).text(s);
        if(! published(e)) {
            published(e, t);
        }
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
        auto title = e.ensurechild(kAtomTitleTag);
        title.attrib().set("type", "text");
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
        auto link = e.ensurechild(kAtomLinkTag);
        link.attrib().set({
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
        e.ensurechild(kAtomIconTag).text(s);
    }

    std::string description(const Element &e) const
    {
        return kAtomSubtitlePath.findtext(e);
    }

    void description(Element &e, const std::string &s) const
    {
        auto subtitle = e.ensurechild(kAtomSubtitleTag);
        subtitle.attrib().set("type", "text");
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


static string
channelFindText_(const Element &e, const XPath &xp)
{
    auto maybe = e.child("channel");
    if(maybe) {
        return xp.findtext(*maybe);
    }
    return "";
}


class Rss20ItemFormat
    : public ItemFormat
{
    public:
    static Rss20ItemFormat instance;

    std::string title(const Element &e) const
    {
        return kRssTitlePath.findtext(e);
    }

    void title(Element &e, const std::string &s) const
    {
        e.ensurechild("title").text(s);
    }

    std::string link(const Element &e) const
    {
        return kRssLinkPath.findtext(e);
    }

    void link(Element &e, const std::string &s) const
    {
        e.ensurechild("link").text(s);
    }

    std::string content(const Element &e) const
    {
        return kRssItemContentPath.findtext(e);
    }

    void content(Element &e, const std::string &s) const
    {
        e.ensurechild("description").text(s);
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
        return kDublinCoreCreatorPath.findtext(e);
    }

    void author(Element &e, const std::string &s) const
    {
        auto creator = e.ensurechild({DUBLIN_CORE_NS, "creator"});
        creator.text(s);
    }

    std::string guid(const Element &e) const
    {
        return kRssItemGuidPath.findtext(e);
    }

    void guid(Element &e, const std::string &s) const
    {
        auto guid = e.ensurechild("guid");
        guid.attrib().set("isPermaLink", "false");
        guid.text(s);
    }

    std::string originalGuid(const Element &e) const
    {
        return guid(e);
    }

    time_t published(const Element &e) const
    {
        return parseRfc822Date_(kRssPublishedPath.findtext(e));
    }

    void published(Element &e, time_t t) const
    {
        auto s = formatRfc822_(t);
        e.ensurechild("pubDate").text(s);
        if(! updated(e)) {
            updated(e, t);
        }
    }

    time_t updated(const Element &e) const
    {
        return parseIso8601Date_(kAtomUpdatedPath.findtext(e));
    }

    void updated(Element &e, time_t t) const
    {
        auto s = formatIso8601_(t);
        e.ensurechild(kAtomUpdatedTag).text(s);
        if(! published(e)) {
            published(e, t);
        }
    }
};


class Rss20FeedFormat
    : public FeedFormat
{
    public:
    static Rss20FeedFormat instance;

    bool identify(const Element &e) const
    {
        return (e.tag() == "rss" &&
                e.get("version", "2.0") == "2.0" &&
                e.child("channel"));
    }

    enum feed_format format() const
    {
        return FORMAT_RSS20;
    }

    std::string title(const Element &e) const
    {
        return channelFindText_(e, kRssTitlePath);
    }

    void title(Element &e, const std::string &s) const
    {
        auto chan = e.ensurechild("channel");
        chan.ensurechild("title").text(s);
        icon(e, icon(e)); // update title.
    }

    std::string link(const Element &e) const
    {
        return channelFindText_(e, kRssLinkPath);
    }

    void link(Element &e, const std::string &s) const
    {
        auto chan = e.ensurechild("channel");
        chan.ensurechild("link").text(s);
        icon(e, icon(e)); // update link.
    }

    std::string description(const Element &e) const
    {
        return channelFindText_(e, kRssItemContentPath);
    }

    void description(Element &e, const std::string &s) const
    {
        auto chan = e.ensurechild("channel");
        chan.ensurechild("description").text(s);
    }

    std::string icon(const Element &e) const
    {
        return kRssIconPath.findtext(e);
    }

    void icon(Element &e, const std::string &s) const
    {
        auto chan = e.ensurechild("channel");
        auto img = chan.ensurechild("image");
        img.ensurechild("title").text(title(e));
        img.ensurechild("link").text(link(e));
        img.ensurechild("url").text(s);
    }

    time_t published(const Element &e) const
    {
        return parseRfc822Date_(kRssPublishedPath.findtext(e));
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
        std::vector<Item> out;
        for(auto &elem : kRssItemsPath.findall(e)) {
            out.push_back(Item(Rss20ItemFormat::instance, elem));
        }
        return out;
    }

    Feed create() const
    {
        auto rss = Element("rss", {
            {"version", "2.0"}
        });
        rss.ensurens(DUBLIN_CORE_NS);
        rss.ensurens(ATOM_NS);
        SubElement(rss, "channel");
        return Feed(Rss20FeedFormat::instance, rss);
    }

    Item append(Element &e) const
    {
        auto chan = e.ensurechild("channel");
        return Item(Rss20ItemFormat::instance,
            SubElement(chan, "item"));
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
