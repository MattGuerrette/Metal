
add_library(base 
    ${CMAKE_CURRENT_SOURCE_DIR}/example.h
    ${CMAKE_CURRENT_SOURCE_DIR}/example.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/simd_matrix.h)

set_target_properties(base PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
target_link_libraries(base 
                      "-framework AppKit"
                      "-framework QuartzCore"
                      "-framework Metal")
