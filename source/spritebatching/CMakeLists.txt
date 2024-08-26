
set(EXAMPLE spritebatch)
set(RESOURCE_FILES "${PROJECT_BINARY_DIR}/generated/default.metallib")

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
endif ()

list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tilesheet.png)

add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatch.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatch.cpp
        ${RESOURCE_FILES})

# Ensure shader library is available
add_dependencies(${EXAMPLE} CompileMetalShaders)

target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${EXAMPLE}
        MACOSX_BUNDLE_GUI_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 14.0
        XCODE_ATTRIBUTE_PRODUCT_NAME ${EXAMPLE}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
