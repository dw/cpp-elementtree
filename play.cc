
#include <cassert>
#include <iostream>
#include <fstream>

#include <time.h>

#include "element.hpp"
#include "feed.hpp"
#include "util.hpp"


using namespace std;
using namespace etree;
using namespace etree::feed;


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


static std::string get_file_contents(const std::string &filename)
{
    std::ifstream fp(filename.c_str(), std::ios::in | std::ios::binary);
    if(! fp) {
        return std::string();
    }

    std::string out;
    fp.seekg(0, std::ios::end);
    out.resize(fp.tellg());
    fp.seekg(0, std::ios::beg);
    fp.read(&out[0], out.size());
    return out;
}

void main3()
{
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
    std::string rss = get_file_contents("red.rss");
    Feed f = etree::feed::fromstring(rss);
    cout << f.title() << endl;
    auto items = f.items();
    cout << items.size() << endl;
    cout << endl;
    for(auto &item : items) {
        cout << item.title() << endl;
        cout << " + " << item.guid() << endl;
        cout << "   " << item.link() << endl;
        time_t c = item.created();
        cout << "   " << ctime(&c) << endl;
        cout << "   " << item.description() << endl;
        cout << endl;
    }
    cout << endl;
}


int main(void)
{
    try {
        main6();
    } catch(std::exception &e) {
        cout << "EEK! " << e.what() << endl;
        abort();
    }
    return 0;
}
