
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
};


static bool rss20Identify_(const Element &e)
{
    return e.tag() == "rss" && e.get("version", "2.0") == "2.0";
}


static Item::content_type rss20GetDescrType_(const Element &e)
{
    return Item::CTYPE_HTML;
}


static void rss20SetDescrType_(Element &e, Item::content_type type)
{
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
        /* itemPubDatePath */       {"pubDate"}
    }
};

static const int formatsCount_ = sizeof formats_ / sizeof formats_[0];


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
    cout << "looking for " << qn << " in " << parent << "\n";
    Nullable<Element> maybe = parent.child(qn);
    cout << "found? " << bool(maybe) << "\n";
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
