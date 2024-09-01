include(FetchContent)

FetchContent_Declare(
        cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG master
)

FetchContent_Declare(sal
        GIT_REPOSITORY "https://github.com/MattGuerrette/sal.git"
        GIT_TAG main
)

FetchContent_Declare(directxmath
        GIT_REPOSITORY "https://github.com/Microsoft/DirectXMath.git"
        GIT_TAG dec2022
)

if (NOT IMGUI_DIR)
    # Use fork as some fixes/changes may be made for macOS/iOS
    # that may not be immediately accepted upstream.
    FetchContent_Declare(imgui
            GIT_REPOSITORY "https://github.com/MattGuerrette/imgui.git"
            GIT_TAG master
    )
    FetchContent_MakeAvailable(imgui)
else ()
    set(imgui_SOURCE_DIR ${IMGUI_DIR})
    include_directories(${imgui_SOURCE_DIR})
endif ()

FetchContent_Declare(stb
        GIT_REPOSITORY "https://github.com/nothings/stb.git"
        GIT_TAG master
)

FetchContent_Declare(sdl3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG main
)

if (NOT METALCPP_DIR)
    FetchContent_Declare(metalcpp
            GIT_REPOSITORY "https://github.com/MattGuerrette/metalcpp"
            GIT_TAG main
    )
    FetchContent_MakeAvailable(metalcpp)
else ()
    add_subdirectory(${METALCPP_DIR} metalcpp)
endif ()

FetchContent_Declare(fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG 10.1.1
)

set(KTX_FEATURE_STATIC_LIBRARY ON)
set(KTX_FEATURE_TOOLS OFF)
set(KTX_FEATURE_TESTS OFF)
if (APPLE)
    set(BASISU_SUPPORT_SSE OFF)
endif ()
FetchContent_Declare(
        ktx
        GIT_REPOSITORY "https://github.com/KhronosGroup/KTX-Software.git"
        GIT_TAG v4.0.0
)
FetchContent_GetProperties(ktx)
if (NOT ktx_POPULATED)
    FetchContent_Populate(ktx)
    add_subdirectory(${ktx_SOURCE_DIR} ${ktx_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(sal directxmath fmt stb sdl3 cgltf)

include_directories(${sal_SOURCE_DIR} ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${stb_SOURCE_DIR} ${metalcpp_SOURCE_DIR} ${directxmath_SOURCE_DIR}/Inc)
