
cmake_minimum_required(VERSION 2.8)

project(cpp-elementtree LANGUAGES CXX)

include(CTest)
enable_testing()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wunused -g -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wunused -g -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
endif()

include_directories(${cpp-elementtree_SOURCE_DIR}/include)

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(test)






