
set(EXAMPLE triangle)

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
            "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Main.storyboard"
            "${CMAKE_CURRENT_SOURCE_DIR}/iOS/Launch Screen.storyboard")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set(RESOURCE_FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib
            ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/LaunchScreen.storyboard)
else ()
    set(XIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/macOS/Application.xib)
    set(RESOURCE_FILES
        ${XIB_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/shader.metal
            ${CMAKE_CURRENT_BINARY_DIR}/shader.metallib)
endif ()
add_executable(${EXAMPLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/Example.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Example.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/main.mm
        ${RESOURCE_FILES})
set_target_properties(${EXAMPLE} PROPERTIES
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")

if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set_target_properties(${EXAMPLE} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/iOS/Info.plist.in)
    target_link_libraries(${EXAMPLE} "-framework UIKit" "-framework GameController")
    target_sources(${EXAMPLE} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/ExampleViewController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/ExampleViewController.m
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/AppDelegate.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/AppDelegate.m)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set_target_properties(${EXAMPLE} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/tvOS/Info.plist.in)
    target_link_libraries(${EXAMPLE} "-framework UIKit")
    target_sources(${EXAMPLE} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/AppDelegate.h
        ${CMAKE_CURRENT_SOURCE_DIR}/iOS/AppDelegate.m)
else()
    source_group("Interface Builder" FILES ${XIB_FILES})
    set_target_properties(${EXAMPLE} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/macOS/Info.plist.in)
    target_sources(${EXAMPLE} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/ExampleViewController.h
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/ExampleViewController.m
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/AppDelegate.h
        ${CMAKE_CURRENT_SOURCE_DIR}/macOS/AppDelegate.m)

    target_link_libraries(${EXAMPLE} "-framework Cocoa")
endif()

target_link_libraries(${EXAMPLE} base)
set_target_properties(${EXAMPLE}
        PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.mpg.triangle
        XCODE_GENERATE_SCHEME YES
        XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY NO
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
        XCODE_ATTRIBUTE_PRODUCT_NAME triangle
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.mpg.triangle
        RESOURCE "${RESOURCE_FILES}")
