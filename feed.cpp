
/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <time.h>

#include "feed.hpp"


namespace etree {
namespace feed {

using namespace etree;
using std::string;


typedef bool (*IdentifyFunc)(const Element &);
typedef std::vector<QName> NameList;

struct Format
{
    std::string name;
    IdentifyFunc identifyFunc;
    NameList titlePath;
    NameList linkPath;
    NameList descrPath;
    NameList entriesPath;
    NameList itemTitlePath;
    NameList itemLinkPath;
    NameList itemDescrPath;
};


static bool identifyRss20(const Element &e)
{
    return e.tag() == "rss" && e.get("version", "2.0") == "2.0";
}


static bool parseRfc822Date(const std::string &s)
{
    
}


static const Format formats_[] = {
    {
        "RSS 2.0",
        /* identifyFunc*/       identifyRss20,
        /* titlePath */         {"channel", "title"},
        /* linkPath */          {"channel", "link"},
        /* descrPath */         {"channel", "description"},
        /* entriesPath */       {"channel", "item"},
        /* itemTitlePath */     {"title"},
        /* itemLinkPath */      {"link"},
        /* itemDescrPath */     {"descr"}
    }
};

static const int formatsCount_ = sizeof formats_ / sizeof formats_[0];


/// ----------------
/// Helper functions
/// ----------------

Entry makeEntry_(const Format &format, Element elem)
{
    return Entry(format, elem);
}


Feed makeFeed_(const Format &format, Element elem)
{
    return Feed(format, elem);
}



/// ---------------
/// Entry functions
/// ---------------

Entry::Entry(const Format &format, Element elem)
    : format_(format)
    , elem_(elem)
{
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
#include <iostream>
using namespace std;

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


std::vector<Entry> Feed::entries() const
{
    Element cur = elem_;
    const NameList &names = format_.entriesPath;

    for(int i = 0; i < (names.size() - 1); i++) {
        cur = getChild_(cur, names[i]);
    }

    std::vector<Element> elems = cur.children(names.back());
    std::vector<Entry> out;
    for(int i = 0; i < elems.size(); i++) {
        Element &elem = elems[i];
        out.push_back(makeEntry_(format_, elem));
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
