cmake_minimum_required(VERSION 3.16)

project(ic_proe_2025 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(proe_brkga
    instance.cpp
    src/decoder/PROEDecoder.cpp
    src/main/main.cpp
)

target_include_directories(proe_brkga PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/decoder
    ${CMAKE_CURRENT_SOURCE_DIR}/src/brkga_mp_ipr_cpp/examples/tsp/src/single_obj/brkga_mp_ipr
)


