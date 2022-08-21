
set(EXAMPLE spritefont)

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
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/iOS/LaunchScreen.storyboard)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
else ()
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/BasicSprite.png
            ${CMAKE_CURRENT_SOURCE_DIR}/Arial.fnt
            ${CMAKE_CURRENT_SOURCE_DIR}/Arial.png
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib)
endif ()
add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
	${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatch.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SpriteBatch.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/SpriteFont.h
	${CMAKE_CURRENT_SOURCE_DIR}/SpriteFont.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/main.mm
        ${RESOURCE_FILES})
if ((${CMAKE_SYSTEM_NAME} MATCHES "iOS") OR (${CMAKE_SYSTEM_NAME} MATCHES "tvOS"))
    target_sources(${EXAMPLE} PRIVATE ${SDL2_SOURCE_DIR}/src/main/uikit/SDL_uikit_main.c)
endif ()
target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.mpg.spritefont
        XCODE_GENERATE_SCHEME YES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.mpg.spritefont
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        RESOURCE "${RESOURCE_FILES}")
