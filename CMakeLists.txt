cmake_minimum_required(VERSION 3.16)
project(NES_Emulator VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# Dependencies
find_package(SDL2 QUIET)

if(TARGET SDL2::SDL2)
    set(SDL2_TARGET SDL2::SDL2)
    message(STATUS "Found local SDL2 installation.")
elseif(TARGET SDL2::SDL2-static)
    set(SDL2_TARGET SDL2::SDL2-static)
    message(STATUS "Found local SDL2-static installation.")
else()
    message(STATUS "SDL2 not found locally. Fetching via FetchContent...")
    
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.0
    )
    
    # Disable building extra components we don't need to speed up compile time
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(SDL2)
    set(SDL2_TARGET SDL2::SDL2-static)
endif()

set(CORE_SOURCES
    src/cartridge/cartridge.cpp
    # List backend cpp relative file paths here
)

set(FRONTEND_SOURCES
    # List frontend cpp relative file paths here
)

set(APP_SOURCES
    src/main.cpp
)

# Target Exe Config
add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
    ${CORE_SOURCES}
    ${FRONTEND_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2::SDL2main
        ${SDL2_TARGET}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)