
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
    
if (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
target_link_libraries(base 
                      "-framework UIKit"
                      "-framework GameController"
                      "-framework QuartzCore"
                      "-framework Metal")
else()
target_link_libraries(base 
                      "-framework AppKit"
                      "-framework GameController"
                      "-framework QuartzCore"
                      "-framework Metal")
endif()
