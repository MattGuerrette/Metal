
set(BASE_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/example.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics_math.h
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/camera.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_x.h
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/simd_matrix.h)

if (${CMAKE_SYSTEM_NAME} MATCHES "tvOS" OR ${CMAKE_SYSTEM_NAME} MATCHES "iOS")
add_library(base ${BASE_SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/example.mm)
else()
add_library(base ${BASE_SOURCE_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/example.mm)
endif()

set_target_properties(base PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION C
	XCODE_GENERATE_SCHEME YES
	XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
 
target_include_directories(base PUBLIC
    ${DirectXMath_SOURCE_DIR}/Inc
    ${DirectXTK_SOURCE_DIR}/Inc)
    
if (${CMAKE_SYSTEM_NAME} MATCHES "tvOS" OR ${CMAKE_SYSTEM_NAME} MATCHES "iOS")
target_link_libraries(base
                      PUBLIC
                      SDL2::SDL2-static
                      "-framework UIKit"
                      "-framework GameController"
                      "-framework QuartzCore"
                      "-framework Metal")
else()
target_link_libraries(base
                      PUBLIC
                      SDL2::SDL2-static
                     # "-framework AppKit"
                      #"-framework GameController"
                      #"-framework QuartzCore"
                      "-framework Metal")
endif()
