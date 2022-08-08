
find_program(XCODE_RUN NAMES xcrun REQUIRED)
if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(XCODE_RUN_SDK iphoneos)
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
            ${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard
            ${CMAKE_CURRENT_SOURCE_DIR}/Example.png)
else ()
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/Example.png)
endif ()
add_executable(triangle
        ${CMAKE_CURRENT_SOURCE_DIR}/triangle.mm
        ${RESOURCE_FILES})
if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    target_sources(triangle
            ${SDL2_SOURCE_DIR}/src/main/uikit/SDL_uikit_main.c)
endif ()
target_link_libraries(triangle base)
set_target_properties(triangle
        PROPERTIES
        MACOSX_BUNDLE TRUE
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/examples/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
