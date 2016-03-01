
# cpp-ElementTree

This libxml2 wrapper is intended as a small experiment in C++ interface design
and various tricks relating to C++11, while still being useful.

It attempts to closely mimic the lxml Python library's <a
href="http://lxml.de/tutorial.html">ElementTree</a> interface and all the perks
that entails (e.g. uniform namespace and text handling), while making use of
some modern C++ design ideas, such as automatic memory and ownership
management. It is intended to work *without surprises*, in other words all
libxml2 management quirks should be hidden.

When built with a C++11 compiler, uniform initializer lists may be used in
various places to specify attribute lists:

````cpp
using etree::Element;
using etree::SubElement;

Element make_status_msg()
{
    Element msg("status", {
        {"first-attribute",          "first-value"},
        {"{urn:some-namepace}attr2", "second-value"}
    });
    SubElement(msg, "system-load").text(get_system_load());
    return msg;
}
````

The ``etree::feed`` namespace includes an incomplete RSS/ATOM feed
parser/generator. It is a work in progress, but can already parse some basic
pieces.


## Text handing

Text passed to the API is assumed to be in UTF-8 format. Text returned by the
API is in UTF-8 format wrapped in a std::string.


## Thread safety

The library and underlying libxml2 implementation are thread safe in that
determistic behavior is guaranteed only so long as library objects aren't
shared among threads. This is due to the library internally performing unlocked
reference counting using the underlying libxml2 DOM structures.

It is only possible to achieve thread safe behaviour if a single thread at a
time is considered owner for all live objects referencing a given document. For
a thread to pass an object to another thread, it must relinquish all remaining
references it holds on that document beforehand.


## Building

cpp-ElementTree consists of two files: ``element.hpp`` and ``element.cpp``. The
implementation file must be linked against libxml2 somehow during the build.

The ``etree::feed`` implementation relies on various POSIX-related time parsing
functions that aren't found on Windows. In order to avoid a large external
dependency (e.g. Boost), ``etree::feed`` is UNIX-only for the time being.


## Horrors

 * Given the choice of repeatedly heap-allocating potentially short-lived
   proxies, or casting a ``void *`` for use as an integer, the latter path was
   chosen.


## TODO

* Remove items from *Horrors* section.
* Use libxml2 parser string interning.
* Preserve namespace prefixes better.
* Disable libxml2 stderr logs (seemingly requires TLS tricks).
* Fix up const usage everywhere (findall/removeall/etc)
