cmake_minimum_required(VERSION 3.16)
project(GeneticoCaixeiroViajante LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # -std=c++17 puro, sem extensões do GNU

# Sem tipo de build informado, usa Release: o AG roda milhões de avaliações e
# sem otimização (-O0) o bônus com muitos pontos ficaria muito mais lento
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

# Biblioteca com todo o núcleo: o executável principal, o de experimentos e os testes
# linkam nela, então cada .cpp é compilado uma vez só
add_library(tsp STATIC
    # bloco A
    src/core/Cenario.cpp
    src/core/Distancia.cpp
    src/io/ArgParser.cpp
    src/io/CsvWriter.cpp
    # bloco B
    src/genetico/Individuo.cpp
    src/genetico/operadores/Torneio.cpp
    src/genetico/operadores/ERX.cpp
    src/genetico/operadores/InversaoMutacao.cpp
    # bloco C
    src/algoritmo/AlgoritmoGenetico.cpp
)
target_include_directories(tsp PUBLIC include)

# Avisos ligados para todo mundo; PUBLIC propaga para quem linka na biblioteca (testes e executáveis)
if(MSVC)
    target_compile_options(tsp PUBLIC /W4)
else()
    target_compile_options(tsp PUBLIC -Wall -Wextra -pedantic)
endif()

# Link estático (combinado na divisão do trabalho): o .exe roda em qualquer Windows
# sem precisar das DLLs do MinGW (libstdc++, libgcc) no PATH
function(linkar_estatico alvo)
    if(MINGW) # no Linux/macOS não é necessário (e no macOS -static nem é suportado)
        target_link_options(${alvo} PRIVATE -static)
    endif()
endfunction()

# Executáveis do bloco C: descomentar quando main.cpp e experimentos.cpp tiverem um main()
# add_executable(tsp_ag src/main.cpp)
# target_link_libraries(tsp_ag PRIVATE tsp)
# linkar_estatico(tsp_ag)
#
# add_executable(experimentos src/experimentos.cpp)
# target_link_libraries(experimentos PRIVATE tsp)
# linkar_estatico(experimentos)

# Testes: um executável por módulo, sem framework externo (ver tests/Assercoes.hpp).
# Cada um retorna 0 se tudo passou; o CTest só olha esse código de saída
enable_testing()

function(adicionar_teste nome arquivo)
    add_executable(${nome} ${arquivo})
    target_link_libraries(${nome} PRIVATE tsp)
    target_include_directories(${nome} PRIVATE tests) # Assercoes.hpp e helpers como genetico/VerificarPermutacao.hpp
    linkar_estatico(${nome})
    add_test(NAME ${nome} COMMAND ${nome})
endfunction()

adicionar_teste(testes_core tests/core/TestesCore.cpp)
adicionar_teste(testes_io tests/io/TestesIo.cpp)
