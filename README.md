
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



### Object Sizes

Element objects are one word, i.e. 8 bytes on 64bit, and may be copied very
cheaply. Copying an element copies only this word and causes a reference count
to be incremented.


### Reference Counting

Element, ElementTree and AttrMap each call ref() during construction and
unref() during destruction.

When called on a non-document node, ref() treats the node's private user data
pointer as an integer and increments it. If it was previously 0, ref() calls
ref() again on the node's document.

When called on a non-documnt node, unref() decrements the pointer. When it
reaches 0, unref() calls unref() again on the node's document.

When unref() on a document node reaches zero, ::xmlFreeDoc() is invoked to
destroy the document.

Due to this approach, it is possible to update a node's associated document
(e.g. during append(), remove(), graft()) without having to update every
Element value in existence, since regardless of how many exist, only one ref()
was ever called on the node document. Once an element has moved to a new
document, the mutation function need only call unref() once on the old document
and ref() once on the new document.


## TODO

* Remove items from *Horrors* section.
* Use libxml2 parser string interning.
* Preserve namespace prefixes better.
* Disable libxml2 stderr logs (seemingly requires TLS tricks).
* Fix up const usage everywhere (findall/removeall/etc)
* Internally copy XPathContext for each thread (e.g. boost::thread_local_ptr)
* etree::tostring() should copy up namespaces to subelements like lxml
* Make child/attr iterators mutation-safe
* Handle comments better.
* Rewrite & better tests for Element::graft().
* reparent() must update refcounts when the document has changed, if there are
  any exitent Elements for deeply nested child nodes.
