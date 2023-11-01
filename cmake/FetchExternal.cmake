include(FetchContent)

FetchContent_Declare(sal
        GIT_REPOSITORY "https://github.com/MattGuerrette/sal.git"
        GIT_TAG main
)

FetchContent_Declare(directxmath
        GIT_REPOSITORY "https://github.com/Microsoft/DirectXMath.git"
        GIT_TAG dec2022
)

FetchContent_Declare(imgui
        GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
        GIT_TAG v1.89.9
)

FetchContent_Declare(stb
        GIT_REPOSITORY "https://github.com/nothings/stb.git"
        GIT_TAG master
)

FetchContent_Declare(sdl2
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG release-2.28.1
)


set(BUILD_SHARED_LIBS OFF)
set(SDL2TTF_INSTALL OFF)
set(SDL2TTF_VENDORED ON)
FetchContent_Declare(sdlttf
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
        GIT_TAG release-2.20.2
)

FetchContent_Declare(metalcpp
        GIT_REPOSITORY "https://github.com/MattGuerrette/metalcpp"
        GIT_TAG main
)


FetchContent_MakeAvailable(sal directxmath stb imgui sdl2 sdlttf metalcpp)

include_directories(${sal_SOURCE_DIR} ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${stb_SOURCE_DIR} ${metalcpp_SOURCE_DIR} ${directxmath_SOURCE_DIR}/Inc)
