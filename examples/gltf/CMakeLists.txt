set(EXAMPLE gltf)

FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG v1.12
)
FetchContent_MakeAvailable(cgltf)

find_program(XCODE_RUN NAMES xcrun REQUIRED)
if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(XCODE_RUN_SDK iphoneos)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set(XCODE_RUN_SDK appletvos)
else ()
    set(XCODE_RUN_SDK macosx)
endif ()
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shader.metallib"
        COMMAND ${XCODE_RUN} -sdk ${XCODE_RUN_SDK} metal -c shader.metal -o ${CMAKE_CURRENT_BINARY_DIR}/shader.air
        COMMAND ${XCODE_RUN} -sdk ${XCODE_RUN_SDK} metallib ${CMAKE_CURRENT_BINARY_DIR}/shader.air -o ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
        COMMENT "Compiling shader to metal library"
        DEPENDS shader.metal
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/Default_albedo.jpg
            ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/Default_albedo.jpg
            ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
else ()
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/single_bone.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/single_bone.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/SimpleSkin.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/skinAnimation.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/skinGeometry.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/skinningData.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/SimpleSkin.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/inverseBindMatrices.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/CesiumMan.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/CesiumMan.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/CesiumMan_img0.jpg
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.gltf
            ${CMAKE_CURRENT_SOURCE_DIR}/DamagedHelmet.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/Default_albedo.jpg
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib)
endif ()
add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/model.h
        ${CMAKE_CURRENT_SOURCE_DIR}/model.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/gltf.mm
        ${RESOURCE_FILES})
target_include_directories(${EXAMPLE}
        PRIVATE
        ${cgltf_SOURCE_DIR})
if ((${CMAKE_SYSTEM_NAME} MATCHES "iOS") OR (${CMAKE_SYSTEM_NAME} MATCHES "tvOS"))
    target_sources(${EXAMPLE} PRIVATE ${SDL2_SOURCE_DIR}/src/main/uikit/SDL_uikit_main.c)
endif ()
target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.mpg.gltf
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_NAME gltf
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.mpg.gltf
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
