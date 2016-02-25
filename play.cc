
#include <algorithm>
#include <stdlib.h>

#include <cassert>
#include <iostream>
#include <fstream>

#include <time.h>

#include "fileutil.hpp"
#include "element.hpp"
#include "feed.hpp"
#include "util.hpp"


#define RASPBERRY_PI "/Users/dmw/src/reader/archive/http___www.raspberrypi.org__feed_rss2.xml.gz"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::ofstream;
using std::ostream;

using namespace etree;
using etree::feed::Feed;


int main2(void)
{
    Element p("dave");
    Element c("{dave.com}dave");
    p.append(c);
    assert(p.size());
    p[0];

    cout << "p.parent? " << bool(p.getparent()) << " sz " << p.size() << endl;
    cout << "c.parent? " << bool(c.getparent()) << " sz " << c.size() << endl;
    cout << endl;

    Element n("zerp");
    //n.append(p);

    AttrMap am = n.attrib();
    dumpVector("attrs", am.keys());
    am.set("dave", "wilson");
    am.set("{}davey", "wilson");
    cout << "dave == " << am.get("dave") << endl;
    cout << "daveums == " << am.get("daveums", "zinky") << endl;

    dumpVector("attrs2", am.keys());
    return 0;
}


void main3()
{
#ifdef ETREE_0X
    Element root("dave", {
        {"zerp", "zoop"}
    });
    /*
    cout << tostring(elem) << endl;
    return;
    
    std::string f = get_file_contents("wr.xml");
    Element root = fromstring(f);
    */
    cout << "---\n";
    Element e = SubElement(root, "donk", {
        { "{zerp}dave", "wilson" }
    });
    cout << "---\n";
    cout << tostring(root);
    cout << "---\n";

    std::string s = tostring(root);
    cout << root << endl;
    cout << "WRITING" << endl;
    cout << s << endl;;
    cout << "WRITING " << s.size() << endl;
#endif
}


void main4()
{
    Element root("{http://zerp.com/}root");
    Element child("{http://blerp.com/}child");

    root.append(child);
}


void main5()
{
    XPath x("/");
    cout << x.expr() << endl;

    Element e = fromstring("<who><name>David</name></who>");
    vector<Element> out = x.findall(e);
    cout << "SIZE: " << out.size() << "\n";


    cout << "name:" << e.findtext("name") << "EEK!" << endl;
    e.find(x);
}



void main6()
{
    ofstream of("/dev/null");
    ostream &os = cout; //of;

    Feed f = etree::feed::fromelement(parse("red.rss").getroot());
    auto items = f.items();
    os << f.format() << endl;
    os << f.title() << endl;
    os << items.size() << endl;
    os << endl;
    for(auto &item : items) {
        os << item.title() << endl;
        os << " + " << item.guid();
        if(item.guid() != item.originalGuid()) {
            os << " " << item.originalGuid();
        }
        os << endl;
        os << "   " << item.link() << endl;
        time_t c = item.published();
        os << "   " << ctime(&c) << endl;
        os << "   " << item.content().substr(0, 80) << endl;
        os << endl;
    }
    os << endl;
}


void prnt(const char *s, const Element &e)
{
    cout << s << " -- " << e.qname() << "\n";
}


Element decompress_xml(const char *path)
{
    std::string xml;
    decompress(path, xml);
    return etree::fromstring(xml.data());
}


void main8a()
{
    Element root = decompress_xml(RASPBERRY_PI);
    prnt("root", root);

    cout << "size " << root.size()<<"\n";

    Nullable<Element> next = root[0];
    while(next) {
        cout << "next: " << *next << "\n";
        next = (*next).getnext();
    }

    return;
    prnt("root[0]", root[0]);
    prnt("root[1]", root[1]);
    prnt("root[2]", root[2]);
    prnt("root[3]", root[3]);
    prnt("root[4]", root[4]);
    prnt("root[169]", root[169]);
    //prnt("root[170]", root[170]);
}


void main8b()
{
    Element root = decompress_xml(RASPBERRY_PI);

    auto it = root.begin();
    cout << bool(it == root.end()) << "\n";
    prnt("it[0]", *it);
    it++;
    prnt("it[1]", *it);
    it++;
    prnt("it[2]", *it);
    it++;
}


void main8()
{
    Element root = decompress_xml(RASPBERRY_PI);

    visit(root, [](Element &e) {
        cout << "hehe: " << e.qname() << "\n";
    });
}


void main9()
{
    Element root = etree::html::fromstring("dave!");
    cout << etree::tostring(root) << "\n";
}


int main(void)
{
    try {
        main9();
    } catch(std::exception &e) {
        cout << "EEK! " << e.what() << endl;
        abort();
    }
    return 0;
}
