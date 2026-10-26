cmake_minimum_required(VERSION 3.16)
project(caixeiro_viajante_genetico LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sem tipo de build explícito o AG roda sem otimização, umas 10x mais lento
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# MinGW/MSYS2: link estático pra o .exe rodar sem as DLLs do MSYS2 no PATH
if(MINGW)
    add_link_options(-static)
endif()

find_package(Threads REQUIRED)

# Tudo menos os main() vira uma biblioteca, reaproveitada pelos executáveis e pelos testes
add_library(tsp_nucleo STATIC
    src/core/Cenario.cpp
    src/core/Distancia.cpp
    src/genetico/Individuo.cpp
    src/genetico/operadores/ERX.cpp
    src/genetico/operadores/InversaoMutacao.cpp
    src/genetico/operadores/Torneio.cpp
    src/algoritmo/Parametros.cpp
    src/algoritmo/AlgoritmoGenetico.cpp
    src/io/ArgParser.cpp
    src/io/CsvWriter.cpp
)
target_include_directories(tsp_nucleo PUBLIC include)

add_executable(tsp src/main.cpp)
target_link_libraries(tsp PRIVATE tsp_nucleo)

add_executable(experimentos src/experimentos.cpp)
target_link_libraries(experimentos PRIVATE tsp_nucleo Threads::Threads)

# Testes: um executável por bloco, cada um devolve != 0 se alguma verificação falhar
enable_testing()
function(adicionar_teste nome fonte)
    add_executable(teste_${nome} ${fonte})
    target_link_libraries(teste_${nome} PRIVATE tsp_nucleo)
    target_include_directories(teste_${nome} PRIVATE tests)
    add_test(NAME ${nome} COMMAND teste_${nome})
endfunction()

adicionar_teste(core tests/core/TesteCore.cpp)
adicionar_teste(io tests/io/TesteIo.cpp)
adicionar_teste(algoritmo tests/algoritmo/TesteAlgoritmo.cpp)
