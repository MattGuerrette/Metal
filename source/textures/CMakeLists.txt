
set(EXAMPLE textures)

if (${CMAKE_GENERATOR} NOT MATCHES "Xcode")
    set(RESOURCE_FILES "${PROJECT_BINARY_DIR}/generated/default.metallib")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
endif ()

list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/001_basecolor.ktx)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/002_basecolor.ktx)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/003_basecolor.ktx)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/004_basecolor.ktx)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/005_basecolor.ktx)


add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${RESOURCE_FILES})

if (${CMAKE_GENERATOR} MATCHES "Xcode")
    target_sources(${EXAMPLE}
            PRIVATE ${CMAKE_SOURCE_DIR}/source/shaders/textures/shader.metal
    )
else ()
    # Ensure shader library is available
    add_dependencies(${EXAMPLE} CompileMetalShaders)
endif ()

target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${EXAMPLE}
        MACOSX_BUNDLE_GUI_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 17.0
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 14.0
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO
        XCODE_ATTRIBUTE_PRODUCT_NAME ${EXAMPLE}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
