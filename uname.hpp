
#ifndef ETREE_UNAME_H
#define ETREE_UNAME_H

#include <stdexcept>
#include <string>

#include "exceptions.hpp"


namespace etree {


class UniversalName {
    std::string ns_;
    std::string tag_;

    void from_string(const std::string &uname);

    public:
    UniversalName(const std::string &ns, const std::string &tag)
        : ns_(ns), tag_(tag) {}
    UniversalName(const UniversalName &other)
        : ns_(other.ns_), tag_(other.tag_) {}
    //UniversalName(UniversalName &&other)
        //: ns_(other.ns_), tag_(other.tag_) {}
    UniversalName(const std::string &uname)
    {
        from_string(uname);
    }
    UniversalName(const char *uname)
    {
        from_string(uname);
    }

    const std::string &tag() const { return tag_; }
    const std::string &ns() const { return ns_; }
    bool operator=(const UniversalName &other)
    {
        return other.tag_ == tag_ && other.ns_ == ns_;
    }
};


std::ostream& operator<< (std::ostream& o, const UniversalName& un);


} // namespace


#endif
