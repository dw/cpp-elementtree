
/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <iostream>

#include "feed.hpp"


namespace etree {
namespace feed {

using namespace etree;
using namespace std;

using std::string;


typedef bool (*IdentifyFunc)(const Element &);
typedef Item::content_type (*GetTypeFunc)(const Element &);
typedef void (*SetTypeFunc)(Element &, Item::content_type);
typedef const std::vector<QName> NameList;

struct Format
{
    std::string name;
    IdentifyFunc identifyFunc;
    GetTypeFunc getDescrTypeFunc;
    SetTypeFunc setDescrTypeFunc;
    NameList titlePath;
    NameList linkPath;
    NameList descrPath;
    NameList itemsPath;
    NameList itemTitlePath;
    NameList itemLinkPath;
    NameList itemDescrPath;
    NameList itemPubDatePath;
    NameList itemGuidPath;
};


/// ----------------
/// Helper functions
/// ----------------

Item makeItem_(const Format &format, Element elem)
{
    return Item(format, elem);
}


Feed makeFeed_(const Format &format, Element elem)
{
    return Feed(format, elem);
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


static bool rss20Identify_(const Element &e)
{
    return e.tag() == "rss" && e.get("version", "2.0") == "2.0";
}


#define ATOM_NS "{http://www.w3.org/2005/Atom}"
static bool atomIdentify_(const Element &e)
{
    return e.qname() == QName(ATOM_NS "feed");
}


static Item::content_type rss20GetDescrType_(const Element &e)
{
    return Item::CTYPE_HTML;
}


static void rss20SetDescrType_(Element &e, Item::content_type type)
{
}


static Item::content_type atomGetDescrType_(const Element &e)
{
    Nullable<Element> content = e.child(ATOM_NS "content");
    if(content) {
        std::string type = (*content).get(ATOM_NS "type");
        if(type == "html" || type == "xhtml") {
            return Item::CTYPE_HTML;
        }
    }
    return Item::CTYPE_TEXT;
}


static void atomSetDescrType_(Element &e, Item::content_type type)
{
    const char *s = (type == Item::CTYPE_HTML) ? "html" : "text";
    getChild_(e, ATOM_NS "content").attrib().set(ATOM_NS "type", s);
}


static const Format formats_[] = {
    {
        "RSS 2.0",
        /* identifyFunc*/           rss20Identify_,
        /* GetDescrTypeFunc */      rss20GetDescrType_,
        /* SetDescrTypeFunc */      rss20SetDescrType_,
        /* titlePath */             {"channel", "title"},
        /* linkPath */              {"channel", "link"},
        /* descrPath */             {"channel", "description"},
        /* itemsPath */             {"channel", "item"},
        /* itemTitlePath */         {"title"},
        /* itemLinkPath */          {"link"},
        /* itemDescrPath */         {"description"},
        /* itemPubDatePath */       {"pubDate"},
        /* itemGuidPath */          {"guid"}
    },
    {
        "ATOM",
        /* identifyFunc*/           atomIdentify_,
        /* GetDescrTypeFunc */      atomGetDescrType_,
        /* SetDescrTypeFunc */      atomSetDescrType_,
        /* titlePath */             {ATOM_NS "title"},
        /* linkPath */              {ATOM_NS "link"},
        /* descrPath */             {ATOM_NS "description"},
        /* itemsPath */             {ATOM_NS "entry"},
        /* itemTitlePath */         {ATOM_NS "title"},
        /* itemLinkPath */          {ATOM_NS "link"},
        /* itemDescrPath */         {ATOM_NS "content"},
        /* itemPubDatePath */       {ATOM_NS "published"},
        /* itemGuidPath */          {ATOM_NS "id"}
    }
};

static const int formatsCount_ = sizeof formats_ / sizeof formats_[0];



/// ---------------
/// Item functions
/// ---------------

Item::Item(const Format &format, Element elem)
    : format_(format)
    , elem_(elem)
{
}


std::string Item::title() const
{
    return getText_(elem_, format_.itemTitlePath);
}


void Item::title(const std::string &s)
{
    setText_(elem_, format_.itemTitlePath, s);
}


std::string Item::link() const
{
    return getText_(elem_, format_.itemLinkPath);
}


void Item::link(const std::string &s)
{
    setText_(elem_, format_.itemLinkPath, s);
}


std::string Item::description() const
{
    return getText_(elem_, format_.itemDescrPath);
}


void Item::description(const std::string &s)
{
    setText_(elem_, format_.itemDescrPath, s);
}


std::string Item::guid() const
{
    return getText_(elem_, format_.itemGuidPath);
}


void Item::guid(const std::string &s)
{
    setText_(elem_, format_.itemGuidPath, s);
}


time_t Item::created() const
{
    std::string s = getText_(elem_, format_.itemPubDatePath);
    if(s.size()) {
        time_t out;
        if(parseRfc822Date_(s, out)) {
            return out;
        }
    }
    return 0;
}


/// --------------
/// Feed functions
/// --------------

Feed::Feed(const Format &format, Element elem)
    : format_(format)
    , elem_(elem)
{
}


Feed::Feed(const Feed &feed)
    : format_(feed.format_)
    , elem_(feed.elem_)
{
}


Feed::~Feed()
{
}


const std::string &Feed::format() const
{
    return format_.name;
}


std::string Feed::title() const
{
    return getText_(elem_, format_.titlePath);
}


void Feed::title(const std::string &s)
{
    setText_(elem_, format_.titlePath, s);
}


std::string Feed::link() const
{
    return getText_(elem_, format_.linkPath);
}


void Feed::link(const std::string &s)
{
    setText_(elem_, format_.linkPath, s);
}


std::string Feed::description() const
{
    return getText_(elem_, format_.descrPath);
}


void Feed::description(const std::string &s)
{
    setText_(elem_, format_.descrPath, s);
}


std::vector<Item> Feed::items() const
{
    Element cur = elem_;
    const NameList &names = format_.itemsPath;

    for(int i = 0; i < (names.size() - 1); i++) {
        cur = getChild_(cur, names[i]);
    }

    std::vector<Element> elems = cur.children(names.back());
    std::vector<Item> out;
    for(int i = 0; i < elems.size(); i++) {
        Element &elem = elems[i];
        out.push_back(makeItem_(format_, elem));
    }
    return out;
}


Feed fromstring(const std::string &s)
{
    Element elem = etree::fromstring(s);
    for(unsigned i = 0; i < formatsCount_; i++) {
        const Format &format = formats_[i];
        if(format.identifyFunc(elem)) {
            return makeFeed_(format, elem);
        }
    }
    throw memory_error();
}


} // namespace
} // namespace
