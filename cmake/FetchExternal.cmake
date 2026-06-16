include(FetchContent)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

if (NOT METALCPP_DIR)
    FetchContent_Declare(metalcpp
            GIT_REPOSITORY "https://github.com/MattGuerrette/metal-cpp"
            GIT_TAG main
    )
    FetchContent_MakeAvailable(metalcpp)
else ()
    add_subdirectory(${METALCPP_DIR} metalcpp)
endif ()

include_directories(${metalcpp_SOURCE_DIR})

set_target_properties(
        metal-cpp
        PROPERTIES FOLDER "External")

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
    add_library(stb::stb ALIAS stb)
endif()

