cmake_minimum_required(VERSION 3.20)
project(ic_proe_brkga LANGUAGES CXX)

# -------------------------------------------------------------
# Configuração geral
# -------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Exibir compilador e flags
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# -------------------------------------------------------------
# Caminhos de include
# -------------------------------------------------------------
# Inclui o diretório onde está o header principal da biblioteca BRKGA
# (src/cpp/brkga_mp_ipr_cpp/brkga_mp_ipr/brkga_mp_ipr.hpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/brkga_mp_ipr_cpp/brkga_mp_ipr)

# Inclui diretórios internos do projeto
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/decoder)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# -------------------------------------------------------------
# Opção de OpenMP (paralelismo opcional)
# -------------------------------------------------------------
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found, enabling parallelism.")
    set(OpenMP_FLAGS ${OpenMP_CXX_FLAGS})
    add_definitions(-DHAS_OPENMP)
else()
    message(STATUS "OpenMP not found, continuing without parallelism.")
endif()

# -------------------------------------------------------------
# Alvo executável: exemplo de teste do BRKGA
# -------------------------------------------------------------
add_executable(test_brkga
    app/test_brkga.cpp
)

# Liga o OpenMP se estiver disponível
if(OpenMP_CXX_FOUND)
    target_link_libraries(test_brkga PRIVATE OpenMP::OpenMP_CXX)
endif()

# -------------------------------------------------------------
# Configuração de warnings
# -------------------------------------------------------------
if (MSVC)
    target_compile_options(test_brkga PRIVATE /W4 /permissive-)
else()
    target_compile_options(test_brkga PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -------------------------------------------------------------
# Mensagens finais
# -------------------------------------------------------------
message(STATUS "BRKGA include path: ${CMAKE_CURRENT_SOURCE_DIR}/brkga_mp_ipr_cpp/brkga_mp_ipr")
message(STATUS "Output binary: ${CMAKE_CURRENT_BINARY_DIR}/test_brkga")

