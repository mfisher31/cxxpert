cmake_minimum_required(VERSION 3.20)
project(cxxpert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
#include <format>
int main() {
    auto s = std::format(\"{}\", 42);
    return 0;
}
" HAVE_STD_FORMAT)

# Each .cpp file in src/ builds to its own CLI program named after the file.
file(GLOB sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
foreach(source IN LISTS sources)
    get_filename_component(name "${source}" NAME_WE)
    add_executable(${name} "${source}")
    if(MSVC)
        target_compile_options(${name} PRIVATE /W4)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
