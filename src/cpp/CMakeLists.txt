cmake_minimum_required(VERSION 3.20)

project(ic_proe_2025_brkga LANGUAGES CXX)

# Padrão de C++ (pode usar 17 também, se preferir)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ----------------------------------------------------------------------
# Includes: onde o compilador vai procurar headers
# ----------------------------------------------------------------------
# diretório raiz deste CMakeLists (src/cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# headers do BRKGA-MP-IPR
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/brkga_mp_ipr_cpp/brkga_mp_ipr)

# seus headers próprios
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/decoder)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------
# OpenMP (opcional, se disponível)
# ----------------------------------------------------------------------
find_package(OpenMP)

if (OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found, enabling parallelism.")
endif()

# ----------------------------------------------------------------------
# Executáveis
# ----------------------------------------------------------------------

# Teste simples com QuadraticDecoder (ou o que estiver em app/test_brkga.cpp)
add_executable(test_brkga
    app/test_brkga.cpp
    # se algum decoder tiver .cpp, adicione aqui, ex:
    # decoder/PROEDecoder.cpp
)

# Se mais tarde você criar um app específico para o PROE, ex:
# add_executable(run_proe
#     app/run_proe.cpp
#     decoder/PROEDecoder.cpp
# )

# ----------------------------------------------------------------------
# Flags de compilação / link
# ----------------------------------------------------------------------
if (OpenMP_CXX_FOUND)
    target_link_libraries(test_brkga PRIVATE OpenMP::OpenMP_CXX)
    # target_link_libraries(run_proe  PRIVATE OpenMP::OpenMP_CXX)
endif()

if (MSVC)
    target_compile_options(test_brkga PRIVATE /W4 /permissive-)
    # target_compile_options(run_proe  PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_brkga PRIVATE -Wall -Wextra -Wpedantic)
    # target_compile_options(run_proe  PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "BRKGA include path: ${CMAKE_CURRENT_SOURCE_DIR}/brkga_mp_ipr_cpp/brkga_mp_ipr")


