
add_library(base STATIC
		Camera.h
		Camera.mm
		Example.h
		Example.mm)

set_target_properties(base PROPERTIES
	XCODE_GENERATE_SCHEME YES
	XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
 
target_include_directories(base PUBLIC
    ${DirectXMath_SOURCE_DIR}/Inc
    ${DirectXTK_SOURCE_DIR}/Inc)
    
if (${CMAKE_SYSTEM_NAME} MATCHES "tvOS" OR ${CMAKE_SYSTEM_NAME} MATCHES "iOS")
target_link_libraries(base
                      PUBLIC
                      SDL2::SDL2-static
                      "-framework MetalKit")
else()
target_link_libraries(base
                      PUBLIC
                      SDL2::SDL2-static
                      "-framework MetalKit")
endif()
