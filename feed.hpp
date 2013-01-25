
#ifndef ETREE_FEED_H
#define ETREE_FEED_H

/*
 * Copyright David Wilson, 2013.
 * License: http://opensource.org/licenses/MIT
 */

#include <ctime>
#include <string>
#include <iostream>

#include "element.hpp"


namespace etree {
namespace feed {


class Format;
class Entry;
class Feed;

Feed fromstring(const std::string &s);
std::string tostring(Feed &feed);
Feed parse(std::istream &is);
Feed parse(const std::string &path);
Feed parse(int fd);


class Entry
{
    protected:
    Element node_;
    Entry(Element node);

    public:
    virtual ~Entry();

    std::string author() const;
    void author(std::string author);

    time_t created() const;
    void created(time_t created);
};


class Feed
{
    protected:
    Element node_;
    const Format &format_;
    Feed(const Format &format, Element node);

    public:
    Feed(const Feed &feed);
    ~Feed();

    std::string title() const;
    void title(const std::string &s);

    std::string link() const;
    void link(const std::string &s);

    std::string description() const;
    void description(const std::string &s);

    std::string author() const;
    void author(const std::string &s);

    std::string icon() const;
    void icon(std::string s);

    time_t published() const;
    void published(time_t t);

    std::vector<Entry> entries() const;
};


} // namespace
} // namespace

#endif
