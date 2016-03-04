#include <iostream>

#include <elementtree/element.hpp>
#include <elementtree/feed.hpp>

int main()
{
    auto doc = etree::parse(std::cin);
    auto feed = etree::feed::fromelement(doc.getroot());

    auto outfeed = etree::feed::create(feed.format());
    outfeed.title(feed.title());
    outfeed.link(feed.link());
    outfeed.description(feed.description());
    outfeed.icon(feed.icon());
    for(auto &item : feed.items()) {
        auto outitem = outfeed.append();
        outitem.title(item.title());
        outitem.link(item.link());
        outitem.type(item.type());
        outitem.content(item.content());
        outitem.author(item.author());
        outitem.guid(item.guid());
        outitem.published(item.published());
        outitem.updated(item.updated());
    }

    std::cout << etree::tostring(outfeed.element());
}
