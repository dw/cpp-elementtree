
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


class Helpers;
class Format;
class Item;
class Feed;

Feed fromstring(const std::string &s);
std::string tostring(Feed &feed);
Feed parse(std::istream &is);
Feed parse(const std::string &path);
Feed parse(int fd);

// Internal.
bool parseRfc822Date_(std::string, time_t &out);


class Item
{
    friend Item makeItem_(const Format &format, Element elem);

    const Format &format_;
    Element elem_;

    Item(const Format &format, Element elem);

    public:
    enum content_type {
        CTYPE_TEXT,
        CTYPE_HTML
    };

    std::string title() const;
    void title(const std::string &s);

    std::string link() const;
    void link(const std::string &s);

    std::string description() const;
    void description(const std::string &s);

    enum content_type description_type() const;
    void description_type(enum content_type);

    std::string author() const;
    void author(std::string author);

    time_t created() const;
    void created(time_t created);
};


class Feed
{
    friend Feed makeFeed_(const Format &, Element);

    protected:
    Element elem_;
    const Format &format_;
    Feed(const Format &format, Element elem);

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

    std::vector<Item> items() const;
};


} // namespace
} // namespace

#endif
