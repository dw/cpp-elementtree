
#include <xapian.h>

#include <condition_variable>
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


#define ARCHIVE "/Users/dmw/src/reader/archive"
#define RASPBERRY_PI "/Users/dmw/src/reader/archive/http___www.raspberrypi.org__feed_rss2.xml.gz"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::ofstream;
using std::ostream;

using namespace etree;
using etree::feed::Feed;

using std::chrono::milliseconds;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;


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
        return duration_cast<milliseconds>(interv).count();
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


typedef std::unique_lock<std::mutex> mutex_lock;

std::atomic_uint_fast64_t items;
std::atomic_uint_fast64_t bytes;
std::atomic_uint_fast64_t files;
std::vector<std::string> paths;
std::mutex paths_mutex;


Element decompress(const std::string &s)
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
    files++;
    bytes += all.size();
    return fromstring(all);
}


void parse_one(const std::string &s)
{
    Feed feed = etree::feed::fromelement(decompress(s));
    items += feed.items().size();
}


class Event
{
    std::condition_variable cond_;
    std::mutex mutex_;
    bool flag_;

    Event(const Event &) = delete;
    Event &operator=(Event &) = delete;

    public:
    Event() : cond_() , flag_(false) {}
    bool is_set() { return flag_; }

    void set() {
        mutex_lock lock(mutex_);
        flag_ = true;
        cond_.notify_all();
    }

    void clear() {
        mutex_lock lock(mutex_);
        flag_ = false;
    }

    void wait() {
        mutex_lock lock(mutex_);
        if(! flag_) {
            cond_.wait(lock, [&]() { return flag_; });
        }
    }

    bool wait(unsigned int timeout) {
        mutex_lock lock(mutex_);
        return flag_ ? flag_ : cond_.wait_for(lock, milliseconds(timeout),
            [&]() { return flag_; });
    }
};


Event empty_event;


void parse_thread()
{
    for(;;) {
        std::string path;
        {
            mutex_lock lock(paths_mutex);
            if(paths.empty()) {
                empty_event.set();
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
    int tcount = 2;

    get_path_list(paths, ARCHIVE);
    cout << "path size: " << paths.size() << "\n";

    timer t;

    std::vector<std::thread*> threads;
    for(int i = 0; i < tcount; i++) {
        std::thread *t1 = new std::thread(parse_thread);
        threads.push_back(t1);
    }

    auto pstat = [&t]() {
        unsigned long long ms = t.ms();
        unsigned long long rate = bytes / std::max(1ULL, ms / 1000) / 1024;
        cout << ms << "ms; done " << files << " files in "
            << bytes << " bytes (" << rate << " kb/sec); "
            << items << " total items\n";
    };

    while(! empty_event.wait(2000)) {
        pstat();
    }

    pstat();
    cout << "join..\n";
    for(auto &th : threads) {
        th->join();
        delete th;
    }
}



void prnt(const char *s, const Element &e)
{
    cout << s << " -- " << e.qname() << "\n";
}

void main8a()
{
    Element root = decompress(RASPBERRY_PI);

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
    Element root = decompress(RASPBERRY_PI);

    auto it = root.begin();
    cout << bool(it == root.end()) << "\n";
    prnt("it[0]", *it);
    it++;
    prnt("it[1]", *it);
    it++;
    prnt("it[2]", *it);
    it++;

    return;

}

void main8()
{
    Element root = decompress(RASPBERRY_PI);

    visit(root, [](Element &e) {
        cout << "hehe: " << e.qname() << "\n";
    });
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
