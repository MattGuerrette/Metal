
set(EXAMPLE instancing)
set(RESOURCE_FILES "${PROJECT_BINARY_DIR}/generated/default.metallib")

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
endif ()

add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/instancing.cpp
        ${RESOURCE_FILES})

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileMetalShaders)

if ((${CMAKE_SYSTEM_NAME} MATCHES "iOS") OR (${CMAKE_SYSTEM_NAME} MATCHES "tvOS"))
    target_sources(${EXAMPLE} PRIVATE ${SDL2_SOURCE_DIR}/src/main/uikit/SDL_uikit_main.c)
endif ()

target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.mpg.instancing
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_NAME instancing
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.MattGuerrette.instancing
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
