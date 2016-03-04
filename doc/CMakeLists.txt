
find_package(doxygen)

if(NOT DOXYGEN_FOUND)
    return()
endif()

configure_file(Doxyfile.in Doxyfile @ONLY)

add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} Doxyfile
    comment "Generating Doxygen documentation" verbatim)
