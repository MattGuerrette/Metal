
file(GLOB EXAMPLES "*/")
list(REMOVE_ITEM EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)
foreach (example ${EXAMPLES})
    get_filename_component(directory ${example} NAME_WLE)
    message(STATUS "Adding example ${directory}")

    add_subdirectory(${example})
endforeach ()