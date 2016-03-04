
find_package(libxml2 REQUIRED)

add_library(elementtree
    element.cpp
    feed.cpp
    feed-util.cpp)

target_include_directories(elementtree PRIVATE ${LIBXML2_INCLUDE_DIR})
target_link_libraries(elementtree PRIVATE ${LIBXML2_LIBRARIES})

add_executable(convert_feed convert_feed.cpp)
target_link_libraries(convert_feed elementtree)

add_executable(sanitize sanitize.cpp)
target_link_libraries(sanitize elementtree)
