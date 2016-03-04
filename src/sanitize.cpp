#include <iostream>
#include <unordered_set>

#include <elementtree/element.hpp>
#include <elementtree/feed.hpp>

static const char tagRemove_[] = (
    "style\0" "script\0" "noscript\0" "object\0"
);

static const char tagWhitelist_[] = (
    "a\0" "abbr\0" "acronym\0" "address\0" "area\0" "article\0" "aside\0"
    "audio\0" "b\0" "blockquote\0" "body\0" "br\0" "button\0" "canvas\0"
    "caption\0" "cite\0" "code\0" "col\0" "colgroup\0" "command\0" "datagrid\0"
    "datalist\0" "dd\0" "del\0" "details\0" "dfn\0" "dialog\0" "dir\0" "div\0"
    "dl\0" "dt\0" "em\0" "event-source\0" "fieldset\0" "figure\0" "font\0"
    "footer\0" "form\0" "h1\0" "h2\0" "h3\0" "h4\0" "h5\0" "h6\0"  "head\0"
    "header\0" "hr\0" "html\0" "i\0" "img\0" "input\0" "ins\0" "kbd\0"
    "keygen\0" "label\0" "legend\0" "li\0" "m\0" "map\0" "menu\0" "meter\0"
    "multicol\0" "nav\0" "nextid\0" "noscript\0" "ol\0" "optgroup\0" "option\0"
    "output\0" "p\0" "pre\0" "progress\0" "q\0" "s\0" "samp\0" "section\0"
    "select\0" "sound\0" "source\0" "spacer\0" "span\0" "strike\0" "strong\0"
    "sub\0" "sup\0" "table\0" "tbody\0" "td\0" "textarea\0" "tfoot\0" "th\0"
    "thead\0" "time\0" "title\0" "tr\0" "tt\0" "u\0" "ul\0" "var\0" "video\0"
);

static const char attrWhitelist_[] = (
    "abbr\0" "accept\0" "accept-charset\0" "accesskey\0" "action\0" "align\0"
    "alt\0" "autoplay\0" "autocomplete\0" "autofocus\0" "axis\0" "balance\0"
    "ch\0" "challenge\0" "char\0" "charoff\0" "choff\0" "charset\0" "checked\0"
    "cite\0" "clear\0" "cols\0" "colspan\0" "compact\0" "contenteditable\0"
    "coords\0" "data\0" "datafld\0" "datapagesize\0" "datasrc\0" "datetime\0"
    "default\0" "delay\0" "dir\0" "disabled\0" "dynsrc\0" "enctype\0" "end\0"
    "face\0" "for\0" "form\0" "frame\0" "galleryimg\0" "gutter\0" "headers\0"
    "height\0" "hidefocus\0" "hidden\0" "high\0" "href\0" "hreflang\0" "icon\0"
    "id\0" "inputmode\0" "ismap\0" "keytype\0" "label\0" "leftspacing\0"
    "lang\0" "list\0" "longdesc\0" "loop\0" "loopcount\0" "loopend\0"
    "loopstart\0" "low\0" "lowsrc\0" "max\0" "maxlength\0" "media\0" "method\0"
    "min\0" "multiple\0" "name\0" "nohref\0" "noshade\0" "nowrap\0" "open\0"
    "optimum\0" "pattern\0" "ping\0" "point-size\0" "prompt\0" "pqg\0"
    "radiogroup\0" "readonly\0" "rel\0" "repeat-max\0" "repeat-min\0"
    "replace\0" "required\0" "rev\0" "rightspacing\0" "rows\0" "rowspan\0"
    "rules\0" "scope\0" "selected\0" "shape\0" "size\0" "span\0" "src\0"
    "start\0" "step\0" "summary\0" "suppress\0" "tabindex\0" "target\0"
    "template\0" "title\0" "toppadding\0" "type\0" "unselectable\0" "usemap\0"
    "urn\0" "valign\0" "value\0" "variable\0" "volume\0" "vrml\0" "width\0"
    "wrap\0"
);


static std::unordered_set<etree::QName>
buildSet_(const char *array, size_t size)
{
    std::unordered_set<etree::QName> set;
    for(auto s = array; s < (array + size); s += 1 + ::strlen(s)) {
        set.insert(s);
    }
    return set;
}


int main()
{
    auto doc = etree::html::parse(std::cin);
    auto tagRemove = buildSet_(tagRemove_, sizeof tagRemove_);
    auto tagWhitelist = buildSet_(tagWhitelist_, sizeof tagWhitelist_);
    auto attrWhitelist = buildSet_(attrWhitelist_, sizeof attrWhitelist_);

    std::vector<etree::Element> all;
    etree::visit(doc.getroot(), [&](etree::Element &e)
    {
        all.push_back(e);
    });

    while(all.size()) {
        auto e = all.back();
        all.pop_back();

        auto tag = e.tag();
        if(tagRemove.count(tag)) {
            e.remove();
            continue;
        }

        if(! tagWhitelist.count(tag)) {
            e.graft();
        }

        for(auto &attr : e.attrib()) {
            auto tag = attr.tag();
            if(! attrWhitelist.count(tag)) {
                e.attrib().remove(tag);
            }
        }
    }

    std::cout << etree::tostring(doc.getroot());
}
