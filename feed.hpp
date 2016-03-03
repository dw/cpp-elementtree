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

class Element;


/**
 * RSS/ATOM feed parser implementation.
 */
namespace feed {

class Feed;
class Item;
class FeedFormat;
class ItemFormat;

// Internal.
time_t parseRfc822Date_(std::string);
time_t parseIso8601Date_(std::string s);
std::string stripWs_(const std::string &s);
std::string
formatIso8601_(time_t t);
std::string
formatRfc822_(time_t t);


/**
 * Enumeration of supported feed types.
 */
enum feed_format {
    /// RSS 2.0.
    FORMAT_RSS20,
    /// ATOM.
    FORMAT_ATOM
};


/**
 * Enumeration of possible content types for an element's content.
 */
enum content_type {
    /// Equivalent to MIME type \c text/plain.
    CTYPE_TEXT,
    /// Equivalent to MIME type \c text/html.
    CTYPE_HTML
};


/**
 * Create a new completely empty feed in the specified format.
 */
Feed create(enum feed_format format);


/**
 * Wrap an Element containing a feed and return a reference.
 *
 * @param elem  Parsed feed as an Element.
 * @returns     Feed reference.
 */
Feed fromelement(Element elem);


Item itemFromElement(Element elem, enum feed_format format);


/**
 * Represent a single feed item. Use Feed::makeItem() to make an instance.
 */
class Item
{
    template<typename T>
    friend FeedFormat &formatFor__(const T &);

    const ItemFormat &format_;
    Element elem_;

    public:
    Item(const Item &);
    Item(const ItemFormat &, const Element &);

    /**
     * Remove this item from its parent feed, if any.
     */
    void remove();

    /**
     * Return the item title.
     */
    std::string title() const;

    /**
     * Set the item title.
     *
     * @param s     New title.
     */
    void title(const std::string &s);

    /**
     * Return the item link URL, or the empty string.
     */
    std::string link() const;

    /**
     * Set the item link URL.
     *
     * @param s     New link.
     */
    void link(const std::string &s);

    /**
     * Return the item content, or the empty string.
     */
    std::string content() const;

    /**
     * Set the item content.
     *
     * @param s     New content.
     */
    void content(const std::string &s);

    /**
     * Return the item content's content type.
     */
    enum content_type type() const;

    /**
     * Set the item content's content type.
     *
     * @param content_type  New content type.
     */
    void type(enum content_type);

    /**
     * Return the item author name, or the empty string.
     */
    std::string author() const;

    /**
     * Set the item author name.
     *
     * @param author
     *      New author name.
     */
    void author(const std::string &author);

    /**
     * Return the item GUID, often this is simply the item URL.
     */
    std::string guid() const;

    /**
     * Set the item GUID.
     *
     * @param s     New item GUID.
     */
    void guid(const std::string &s);

    /**
     * Return the item's GUID as it appeared in the original source feed, or
     * the present GUID if no original GUID is found.
     *
     * \note Used to recover GUIDs from the Google Reader API.
     */
    std::string originalGuid() const;

    /**
     * Return the item's published date as a UNIX timestamp.
     */
    time_t published() const;

    /**
     * Set the item's published date as a UNIX timestamp.
     *
     * @param published
     *      Published date.
     */
    void published(time_t published);

    /**
     * Return the item's updated date as a UNIX timestamp.
     */
    time_t updated() const;

    /**
     * Set the item's updated date as a UNIX timestamp.
     *
     * @param updated
     *      updated date.
     */
    void updated(time_t updated);

    Element element() const;
};


/**
 * Represents a feed. Use etree::makeFeed() to create a blank feed.
 */
class Feed
{
    template<typename T>
    friend FeedFormat &formatFor__(const T &);

    const FeedFormat &format_;
    Element elem_;

    public:
    Feed();
    Feed(const Feed &);
    Feed(const FeedFormat &format, const Element &elem);

    /**
     * Return the format of this feed.
     */
    enum feed_format format() const;

    /**
     * Fetch the feed title.
     */
    std::string title() const;

    /**
     * Set the feed title.
     */
    void title(const std::string &s);

    /**
     * Fetch the feed's link, which is usually its associated web site or topic
     * page.
     */
    std::string link() const;

    /**
     * Set the feed's link.
     */
    void link(const std::string &s);

    /**
     * Fetch the feed description.
     */
    std::string description() const;

    /**
     * Set the feed description.
     */
    void description(const std::string &s);

    /**
     * Fetch the feed icon URL.
     */
    std::string icon() const;

    /**
     * Set the feed icon URL.
     */
    void icon(const std::string &s);

    /**
     * Fetch a vector of all the feed's items.
     */
    std::vector<Item> items() const;

    /**
     * Create an empty item in the correct format and append it to this feed.
     * Avoids a potentially redundant conversion in the case of populating a
     * brand new feed.
     *
     * @returns
     *      The new item.
     */
    Item append();

    /**
     * Append an item to this feed, removing it from its present feed. If the
     * destination feed differs in format from the item, the item is converted
     * in-place, discarding any non-essential data.
     *
     * @param item
     *      Item to append.
     */
    void append(Item item);

    /**
     * Fetch the underlying etree::Element representing the feed.
     */
    Element element() const;
};


} // namespace
} // namespace

#endif
