
#ifndef ETREE_EXCEPTIONS_H
#define ETREE_EXCEPTIONS_H

#include <stdexcept>


namespace etree {


struct uname_error : public std::runtime_error
{
    uname_error() : std::runtime_error("uname error") {}
};


struct serialization_error : public std::runtime_error
{
    serialization_error() : std::runtime_error("serialization error") {}
};


struct element_error : public std::runtime_error
{
    element_error(const char *s) : std::runtime_error(s) {}
};


} // namespace


#endif
