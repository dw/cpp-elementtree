
#ifndef ELEMENTTREE_STDINC_HPP
#define ELEMENTTREE_STDINC_HPP

// Standard library inclusions

#if __cpp_lib_experimental_optional
#include <experimental/optional>
namespace etree {
    using std::experimental::optional;
    using std::experimental::nullopt;
}
#else
#include "extern/optional.hpp"
#endif

#endif
