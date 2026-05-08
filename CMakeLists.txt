cmake_minimum_required(VERSION 4.3.1)

project(JK-Calcubrute)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

#add_library(JK-Calcubrute
#SHARAED
#    src/Context.cpp
#)

add_executable(test
    src/Context.c
    src/Memory.c
    Main.c
)

target_include_directories(test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test
PRIVATE
    "C:/msys64/clang64/lib/libc++.a"
    "C:/msys64/clang64/lib/libc++abi.a"
    "C:/msys64/clang64/lib/libvolk.a"
)

target_compile_definitions(test
PRIVATE
    $<$<CONFIG:Debug>:JK_DEBUG>
)

target_compile_options(test
PRIVATE
    #-ferror-limit=1
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3 -flto -march=core-avx2>
)
