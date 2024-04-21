
set(EXAMPLE distancefieldfont)
set(RESOURCE_FILES "${PROJECT_BINARY_DIR}/generated/default.metallib")

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
endif ()

add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${RESOURCE_FILES})

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileMetalShaders)

if ((${CMAKE_SYSTEM_NAME} MATCHES "iOS") OR (${CMAKE_SYSTEM_NAME} MATCHES "tvOS"))
    target_sources(${EXAMPLE} PRIVATE ${SDL2_SOURCE_DIR}/src/main/uikit/SDL_uikit_main.c)
endif ()

target_link_libraries(${EXAMPLE} base SDL3_ttf::SDL3_ttf-static)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_NAME ${EXAMPLE}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
