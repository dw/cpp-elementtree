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
void stripWs_(std::string &s);


/**
 * Parse a feed from a STL string and return a reference.
 *
 * @param s     Serialized feed.
 * @returns     Feed reference.
 */
Feed fromstring(const std::string &s);


/**
 * Serialize a feed to XML and return it as a STL string.
 *
 * @param feed  Feed to serialize.
 * @returns     Feed serialized as XML.
 */
std::string tostring(Feed &feed);


/**
 * Parse a feed from a STL istream and return a reference.
 *
 * @param is    Source istream.
 * @returns     Feed reference.
 */
Feed parse(std::istream &is);


/**
 * Parse a feed stored in the filesystem and return a reference.
 *
 * @param path  Filesystem path.
 * @returns     Feed reference.
 */
Feed parse(const std::string &path);


/**
 * Parse a feed from a file descriptor and return a reference.
 *
 * @param fd    File descripto.
 * @returns     Feed reference.
 */
Feed parse(int fd);


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
 * Represent a single feed item. Use Feed::makeItem() to make an instance.
 */
class Item
{
    const ItemFormat &format_;
    Element elem_;

    public:
    Item(const Item &);
    Item(const ItemFormat &, const Element &);

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
     * @param s     New author name.
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
     * @param created       Published date.
     */
    void published(time_t published);
};


/**
 * Represents a feed. Use etree::makeFeed() to create a blank feed.
 */
class Feed
{
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

    std::string title() const;
    void title(const std::string &s);

    std::string link() const;
    void link(const std::string &s);

    std::string description() const;
    void description(const std::string &s);

    std::string icon() const;
    void icon(std::string s);

    std::vector<Item> items() const;
};


} // namespace
} // namespace

#endif
