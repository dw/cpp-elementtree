
/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include "feed.hpp"


namespace etree {
namespace feed {

using namespace etree;
using std::string;


typedef bool (*IdentifyFunc)(const Element &);
typedef std::vector<QName> NameList;

struct Format
{
    IdentifyFunc identifyFunc;
    NameList titlePath;
    NameList linkPath;
    NameList descriptionPath;
    NameList entriesPath;
};


static bool identifyRss20(const Element &e)
{
    return e.tag() == "rss" && e.get("version", "2.0") == "2.0";
}


static const Format rss20Format_ = {
    /* identifyFunc*/   identifyRss20,
    /* titlePath */     {"channel", "title"},
    /* linkPath */      {"channel", "link"},
    /* descrPath */     {"channel", "description"},
    /* entriesPath */   {"channel", "item"}
};


Feed::Feed(const Format &format, Element node)
    : format_(format)
    , node_(node)
{
}


Feed::Feed(const Feed &feed)
    : format_(feed.format_)
    , node_(feed.node_)
{
}


Feed::~Feed()
{
}


static Element getChild_(Element &parent, const string &tag)
{
    Nullable<Element> maybe = parent.find(tag);
    return maybe ? *maybe : SubElement(parent, tag);
}


struct Rss2Entry : public Entry
{
    Rss2Entry(Element node)
        : Entry(node)
    {
    }
};


static std::string getText_(const Element &cur, const NameList &names)
{
    for(int i = 0; i < names.size(); i++) {
        Nullable<Element> maybe = cur.child(names[i]);
        if(! maybe) {
            return "";
        }
        cur = *maybe;
    }
    return cur.text();
}


static void setText_(Element &cur, const NameList &names, const std::string &s)
{
    for(int i = 0; i < names.size(); i++) {
        cur = getChild_(cur, names[i]);
    }
    cur.text(s);
}


std::string Feed::title() const
{
    return getText_(node_, format_.titlePath);
}


void Feed::title(const std::string &s)
{
    setText_(node_, format_.titlePath, s);
}


std::string Feed::link() const
{
    return getText_(node_, format_.linkPath);
}


void Feed::link(const std::string &s)
{
    setText_(node_, format_.linkPath, s);
}


std::string Feed::description() const
{
    return getText_(node_, format_.descrPath);
}


void Feed::description(const std::string &s)
{
    setText_(node_, format_.descrPath, s);
}


std::vector<Entry> Feed::entries() const
{
    Element cur = node_;
    const NameList &names = format_.entriesPath;

    for(int i = 0; i < (names.size() - 1); i++) {
        cur = getChild_(cur, names[i]);
    }

    std::vector<Element> elems = cur.children(names.back());
    std::vector<Entry> out;
    for(int i = 0; i < elems.size(); i++) {
        out.push_back(Entry(elems[i]));
    }
    return out;
}


Feed fromstring(const std::string &s)
{
    Element root = etree::fromstring(s);
}


} // namespace
} // namespace
