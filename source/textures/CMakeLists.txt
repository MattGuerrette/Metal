
set(EXAMPLE textures)

if (NOT CMAKE_GENERATOR MATCHES "Xcode")
    set(RESOURCE_FILES "${PROJECT_BINARY_DIR}/generated/default.metallib")
else ()
    set(RESOURCE_FILES "")
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
endif ()

list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/001_basecolor.png)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/002_basecolor.png)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/003_basecolor.png)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/004_basecolor.png)
list(APPEND RESOURCE_FILES ${CMAKE_SOURCE_DIR}/assets/textures/005_basecolor.png)


add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${RESOURCE_FILES})


if (CMAKE_GENERATOR MATCHES "Xcode")
    target_sources(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR}/shaders/textures/shader.metal)
    set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/shaders/textures/shader.metal PROPERTIES
            LANGUAGE METAL
            COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/shaders/textures"
    )

else ()
    # Ensure shader library is available
    add_dependencies(${EXAMPLE} CompileMetalShaders)
endif ()


target_link_libraries(${EXAMPLE} base stb::stb)
target_compile_definitions(${EXAMPLE} PRIVATE USE_STB_IMAGE)

set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME ${EXAMPLE}
        MACOSX_BUNDLE_GUI_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO
        XCODE_ATTRIBUTE_PRODUCT_NAME ${EXAMPLE}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.MattGuerrette.${EXAMPLE}
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
