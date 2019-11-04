
add_executable(triangle ${CMAKE_CURRENT_SOURCE_DIR}/main.mm)
target_link_libraries(triangle base)
set_target_properties(triangle
    PROPERTIES
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/examples/Info.plist.in)