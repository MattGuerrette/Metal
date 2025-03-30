include(FetchContent)

if (NOT METALCPP_DIR)
    FetchContent_Declare(metalcpp
            GIT_REPOSITORY "https://github.com/MattGuerrette/metalcpp"
            GIT_TAG main
    )
    FetchContent_MakeAvailable(metalcpp)
else ()
    add_subdirectory(${METALCPP_DIR} metalcpp)
endif ()

include_directories(${metalcpp_SOURCE_DIR})

set_target_properties(
        AppleFrameworksCpp
        PROPERTIES FOLDER "External")

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/MattGuerrette/imgui.git
    GIT_TAG master
)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)