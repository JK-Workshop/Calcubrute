cmake_minimum_required(VERSION 4.3.1)

project(JK-Calcubrute)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

add_library(Calcubrute
STATIC
    src/Context.c
    src/Memory.c
)

target_include_directories(Calcubrute
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Calcubrute
PRIVATE
    "C:/msys64/clang64/lib/libvolk.a"
)

target_compile_definitions(Calcubrute
PUBLIC
    $<$<CONFIG:Debug>:
        JK_DEBUG
    >
    $<$<CONFIG:Release>:
    >
)

target_compile_options(Calcubrute
PUBLIC
    #-ferror-limit=1
    $<$<CONFIG:Debug>:
        -O0
        -g
    >
    $<$<CONFIG:Release>:
        -O3
        -flto
        -march=core-avx2
        -Rpass=inline
        -Rpass-missed=inline
        -Rpass-analysis=inline
    >
)

add_executable(test Main.c)
target_link_libraries(test PRIVATE Calcubrute)
