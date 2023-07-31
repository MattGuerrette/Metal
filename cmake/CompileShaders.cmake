
# Find appropriate xcrun and sdk based on platform

find_program(XCODE_RUN NAMES xcrun REQUIRED)
if (${CMAKE_SYSTEM_NAME} MATCHES "iOS")
    set(XCODE_RUN_SDK iphoneos)
    set(XCODE_METAL_EXTRA_ARGS "-mios-version-min=16.0")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "tvOS")
    set(XCODE_RUN_SDK appletvos)
    set(XCODE_METAL_EXTRA_ARGS "-mios-version-min=16.0")
else ()
    set(XCODE_RUN_SDK macosx)
    set(XCODE_METAL_EXTRA_ARGS "")
endif ()


# The following function implements support for compiling multiple
# Metal shaders into a single library file. It achieves this using the following steps:
#
# 1) First, compile all shaders specified in the input list into air object files
# 2) Second, take all air file and bundle them using metal-ar into a single package
# 3) Finally, generate a single uber library containing all compiled shaders
function(CompileMetalShaders SHADER_FILES)
    add_custom_target(CompileMetalShaders
            DEPENDS "${PROJECT_BINARY_DIR}/generated/default.metallib"
    )

    set(COMPILED_METAL_AIR_FILES "")
    foreach (file ${SHADER_FILES})
        get_filename_component(name ${file} NAME_WLE)
        get_filename_component(directory ${file} DIRECTORY)
        get_filename_component(dirname ${directory} NAME_WLE)
        list(APPEND COMPILED_METAL_AIR_FILES ${PROJECT_BINARY_DIR}/generated/${dirname}/${name}.air)
        add_custom_command(
                OUTPUT "${PROJECT_BINARY_DIR}/generated/${dirname}/${name}.air"
                COMMAND ${XCODE_RUN} -sdk ${XCODE_RUN_SDK} metal -c ${file} ${XCODE_METAL_EXTRA_ARGS} -o ${PROJECT_BINARY_DIR}/generated/${dirname}/${name}.air
                COMMENT "Compiling ${name} to METAL-AIR"
                DEPENDS ${file} ${directory})
    endforeach ()

    add_custom_command(
            OUTPUT "${PROJECT_BINARY_DIR}/generated/default.metallib"
            COMMENT "Compiling ${COMPILED_METAL_AIR_FILES}"
            COMMAND ${XCODE_RUN} -sdk ${XCODE_RUN_SDK} metal-ar r ${PROJECT_BINARY_DIR}/generated/default.metal-ar ${COMPILED_METAL_AIR_FILES}
            COMMAND ${XCODE_RUN} -sdk ${XCODE_RUN_SDK} metallib -o ${PROJECT_BINARY_DIR}/generated/default.metallib ${PROJECT_BINARY_DIR}/generated/default.metal-ar
            DEPENDS ${COMPILED_METAL_AIR_FILES}
    )
endfunction()