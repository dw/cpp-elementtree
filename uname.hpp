
#include <stdexcept>
#include <string>


namespace etree {


struct uname_error : public std::runtime_error
{
    uname_error() : std::runtime_error("uname error") {}
};


class UniversalName {
    std::string ns_;
    std::string tag_;

    public:
    UniversalName(const std::string &ns, const std::string &tag)
        : ns_(ns), tag_(tag) {}
    UniversalName(const UniversalName &other)
        : ns_(other.ns_), tag_(other.tag_) {}
    UniversalName(UniversalName &&other)
        : ns_(other.ns_), tag_(other.tag_) {}
    UniversalName(const std::string &uname)
    {
        if(uname.size() > 0 && uname[0] == '{') {
            size_t e = uname.find('}');
            if(e == std::string::npos) {
                throw uname_error();
            } else if(uname.size() - 1 == e) {
                throw uname_error();
            }
            ns_ = uname.substr(1, e - 1);
            tag_ = uname.substr(e + 1);
            if(tag_.size() == 0) {
                throw uname_error();
            }
        } else {
            ns_ = "";
            tag_ = uname;
        }
    }

    const std::string &tag() const { return tag_; }
    const std::string &ns() const { return ns_; }
    UniversalName(const char *uname) : UniversalName(std::string(uname)) {}
    bool operator=(const UniversalName &other)
    {
        return other.tag_ == tag_ && other.ns_ == ns_;
    }
};


std::ostream& operator<< (std::ostream& o, const UniversalName& un)
{
    if(un.ns().size()) {
        o << "{" << un.ns() << "}";
    }
    o << un.tag();
    return o;
}


}
