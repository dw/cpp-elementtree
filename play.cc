
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <dirent.h>
#include <chrono>

#include <cassert>
#include <iostream>
#include <fstream>

#include <time.h>
#include <zlib.h>

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


using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

struct timer {
    typedef std::chrono::high_resolution_clock clock_type;

    clock_type clock;
    clock_type::time_point start;

    timer() : clock()
    {
        reset();
    }

    void reset()
    {
        start = clock.now();
    }

    clock_type::duration::rep ms()
    {
        clock_type::time_point end = clock.now();
        clock_type::duration interv = end - start;
        return duration_cast<std::chrono::milliseconds>(interv).count();
    }

    clock_type::duration::rep us()
    {
        clock_type::time_point end = clock.now();
        clock_type::duration interv = end - start;
        return duration_cast<std::chrono::microseconds>(interv).count();
    }
};



void get_path_list(std::vector<std::string> &out, const char *dirname)
{
    DIR *dir = opendir(dirname);
    assert(dir);

    struct dirent *ent;
    while((ent = readdir(dir)) != NULL) {
        if(ent->d_name[0] == '.') {
            continue;
        }
        out.emplace_back();
        std::string &s = out.back();
        s.append(dirname);
        s.append("/");
        s.append(ent->d_name);
    }

    closedir(dir);
}


typedef std::lock_guard<std::mutex> mutex_guard;

std::atomic_uint_fast64_t items;
std::atomic_uint_fast64_t bytes;
std::atomic_uint_fast64_t files;
std::vector<std::string> paths;
std::mutex paths_lock;


void parse_one(std::string s)
{
    gzFile gfp = gzopen(s.c_str(), "r");
    assert(gfp);

    string all;
    string buf(1048576, '\0');

    for(;;) {
        int ret = gzread(gfp, reinterpret_cast<void *>(&buf[0]), buf.size());
        if(ret == -1 || ret == 0) {
            break;
        }
        all.append(buf, 0, ret);
    }
    gzclose(gfp);

    Feed feed = etree::feed::fromelement(fromstring(all));
    items += feed.items().size();
    files++;
    bytes += all.size();
}


void parse_thread()
{
    for(;;) {
        std::string path;
        {
            mutex_guard guard(paths_lock);
            if(paths.empty()) {
                return;
            }
            path = paths.back();
            paths.pop_back();
        }
        parse_one(path);
    }
}


void main7()
{
    get_path_list(paths, "/Users/dmw/src/reader/archive");
    cout << "path size: " << paths.size() << "\n";

    timer t;
    std::chrono::milliseconds dura( 2000 );

    std::thread t1(parse_thread);
    std::thread t2(parse_thread);

    auto pstat = [&t]() {
        unsigned long long ms = t.ms();
        unsigned long long rate = bytes / max(1ULL, ms / 1000) / 1024;
        cout << ms << "ms; done " << files << " files in "
            << bytes << " bytes (" << rate << " kb/sec); "
            << items << " total items\n";
    };

    while(paths.size()) {
        std::this_thread::sleep_for(dura);
        pstat();
    }

    pstat();
    cout << "join..\n";
    t1.join();
    t2.join();
}


int main(void)
{
    try {
        main7();
    } catch(std::exception &e) {
        cout << "EEK! " << e.what() << endl;
        abort();
    }
    return 0;
}
