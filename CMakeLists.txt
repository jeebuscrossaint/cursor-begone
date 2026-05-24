cmake_minimum_required(VERSION 3.16)
project(cursor-begone CXX)

set(CMAKE_CXX_STANDARD 17)
enable_language(RC)

add_executable(cursor-begone WIN32 cursor_hider.cpp cursor_hider.rc)

target_link_libraries(cursor-begone PRIVATE user32 shell32 advapi32)

# Prefer MSVC-style warnings but don't break MinGW builds
if(MSVC)
    target_compile_options(cursor-begone PRIVATE /W3 /O1 /GS- /GR-)
    target_link_options(cursor-begone PRIVATE /OPT:REF /OPT:ICF)
else()
    target_compile_options(cursor-begone PRIVATE
        -Os -fno-exceptions -fno-rtti
        -ffunction-sections -fdata-sections)
    target_link_options(cursor-begone PRIVATE
        -s -Wl,--gc-sections -Wl,--strip-all)
endif()
