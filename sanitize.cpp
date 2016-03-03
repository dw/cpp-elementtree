#include <iostream>
#include <unordered_set>
#include "element.hpp"
#include "feed.hpp"


int main()
{
    auto doc = etree::html::parse(std::cin);
    std::unordered_set<etree::QName> tagRemove {
        "style", "script", "noscript", "object"
    };

    std::unordered_set<etree::QName> tagWhitelist {
        "a", "abbr", "acronym", "address", "area", "article", "aside", "audio",
        "b", "blockquote", "br", "button", "canvas", "caption", "cite", "code",
        "col", "colgroup", "command", "datagrid", "datalist", "dd", "del",
        "details", "dfn", "dialog", "dir", "div", "dl", "dt", "em",
        "event-source", "fieldset", "figure", "footer", "font", "form",
        "header", "h1", "h2", "h3", "h4", "h5", "h6", "hr", "i", "img",
        "input", "ins", "keygen", "kbd", "label", "legend", "li", "m", "map",
        "menu", "meter", "multicol", "nav", "nextid", "noscript", "ol",
        "output", "optgroup", "option", "p", "pre", "progress", "q", "s",
        "samp", "section", "select", "sound", "source", "spacer", "span",
        "strike", "strong", "sub", "sup", "table", "tbody", "td", "textarea",
        "time", "tfoot", "th", "thead", "tr", "tt", "u", "ul", "var", "video",
        "html", "body"
    };
    std::unordered_set<etree::QName> attrWhitelist {
        "abbr", "accept", "accept-charset", "accesskey", "action", "align",
        "alt", "autoplay", "autocomplete", "autofocus", "axis", "balance",
        "ch", "challenge", "char", "charoff", "choff", "charset", "checked",
        "cite", "clear", "cols", "colspan", "compact", "contenteditable",
        "coords", "data", "datafld", "datapagesize", "datasrc", "datetime",
        "default", "delay", "dir", "disabled", "dynsrc", "enctype", "end",
        "face", "for", "form", "frame", "galleryimg", "gutter", "headers",
        "height", "hidefocus", "hidden", "high", "href", "hreflang", "icon",
        "id", "inputmode", "ismap", "keytype", "label", "leftspacing", "lang",
        "list", "longdesc", "loop", "loopcount", "loopend", "loopstart", "low",
        "lowsrc", "max", "maxlength", "media", "method", "min", "multiple",
        "name", "nohref", "noshade", "nowrap", "open", "optimum", "pattern",
        "ping", "point-size", "prompt", "pqg", "radiogroup", "readonly", "rel",
        "repeat-max", "repeat-min", "replace", "required", "rev",
        "rightspacing", "rows", "rowspan", "rules", "scope", "selected",
        "shape", "size", "span", "src", "start", "step", "summary", "suppress",
        "tabindex", "target", "template", "title", "toppadding", "type",
        "unselectable", "usemap", "urn", "valign", "value", "variable",
        "volume", "vrml", "width", "wrap"
    };

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

        std::vector<etree::QName> toRemove;
        for(auto &attr : e.attrib()) {
            auto tag = attr.tag();
            if(! attrWhitelist.count(tag)) {
                toRemove.push_back(tag);
            }
        }

        for(auto &qn : toRemove) {
            e.attrib().remove(qn);
        }
    }

    std::cout << etree::tostring(doc.getroot());
}
