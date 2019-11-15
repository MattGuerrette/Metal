project(coretext)

set(RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal)
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.mm
                        ${RESOURCE_FILES})
target_link_libraries(${PROJECT_NAME} base)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/examples/Info.plist.in
    RESOURCE "${RESOURCE_FILES}")
