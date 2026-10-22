#pragma once

#include <iostream>

namespace tsp::testes {
    /* Mini "framework" de teste: cada verificação imprime ok/FALHA e conta as falhas.
    O main de cada teste devolve resultadoFinal(), que o ctest lê como passou (0) ou falhou (1) */
    inline int falhas = 0;

    inline void verificar(bool condicao, const char* descricao) {
        std::cout << (condicao ? "[ok]    " : "[FALHA] ") << descricao << '\n';
        if (!condicao) ++falhas;
    }

    inline int resultadoFinal() {
        std::cout << (falhas == 0 ? "todos os testes passaram\n" : "houve falhas\n");
        return falhas == 0 ? 0 : 1;
    }
}
