cmake_minimum_required(VERSION 3.15)
project(Fractal)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# Adding external dependencies by including their subdirectories.
add_subdirectory(thirdparty/bgfx.cmake)

# Adding project source directories.
add_subdirectory(Fractal_OpenGL/src)

# add a target that uses shaderc if needed.
# add_subdirectory(src/thirdparty/bgfx.cmake/bgfx/tools/shaderc)

# Defining executable targets 
add_executable(Fractal_OpenGL/src/main.cpp)
target_include_directories(Fractal_OpenGL PRIVATE src/)
target_link_libraries(fractal bgfx bx bimg)