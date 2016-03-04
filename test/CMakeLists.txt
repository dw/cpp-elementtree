
add_executable(test_main
    test_main.cpp
    test_attrib.cpp
    test_element.cpp
    test_feed.cpp
    test_nullable.cpp
    test_parse.cpp
    test_qname.cpp
    test_xpath.cpp)

target_link_libraries(test_main PRIVATE elementtree)

#This copies the contents of testdata/ to the build directory at configure time
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/testdata
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})