
#include "uname.hpp"


namespace etree {


std::ostream& operator<< (std::ostream& o, const UniversalName& un)
{
    if(un.ns().size()) {
        o << "{" << un.ns() << "}";
    }
    o << un.tag();
    return o;
}


void UniversalName::from_string(const std::string &uname)
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


} // namespace
