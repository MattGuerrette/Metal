
set(EXAMPLE skinning)

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

list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/alien-bug.glb)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/skinAnimation.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/skinGeometry.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/skinningData.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/inverseBindMatrices.bin)
list(APPEND RESOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/SimpleSkin.gltf)

add_executable(${EXAMPLE}
        main.cpp
        GLTFAsset.cpp
        ${RESOURCE_FILES})

if (CMAKE_GENERATOR MATCHES "Xcode")
    target_sources(${EXAMPLE} PRIVATE ${CMAKE_SOURCE_DIR}/shaders/skinning/shader.metal)
    set_source_files_properties(
            ${CMAKE_SOURCE_DIR}/shaders/skinning/shader.metal PROPERTIES
            LANGUAGE METAL
            COMPILE_OPTIONS "-I${CMAKE_SOURCE_DIR}/shaders/skinning"
    )
else ()
    # Ensure shader library is available
    add_dependencies(${EXAMPLE} CompileMetalShaders)
endif ()

target_link_libraries(${EXAMPLE} base)
target_include_directories(${EXAMPLE} PRIVATE ${cgltf_SOURCE_DIR})
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
