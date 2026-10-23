#pragma once

#include <cmath>
#include <iostream>

namespace tsp::testes {
    /* Mini framework de teste, sem biblioteca externa (não precisa baixar nada e não briga
    com o link -static). Cada executável de teste chama verificar() várias vezes e retorna
    resultado() no main: 0 se tudo passou, 1 se algo falhou, que é o que o CTest lê */

    inline int falhas = 0; // inline (C++17): uma única variável mesmo incluída em vários .cpp

    inline void verificar(bool condicao, const char* descricao) {
        if (condicao) {
            std::cout << "[ OK ] " << descricao << '\n';
        } else {
            std::cout << "[FALHOU] " << descricao << '\n';
            ++falhas;
        }
    }

    // double quase nunca é exatamente igual depois de contas com sqrt/sin, compara com tolerância
    inline bool quaseIgual(double a, double b, double tolerancia = 1e-9) {
        return std::fabs(a - b) <= tolerancia;
    }

    inline int resultado() {
        std::cout << (falhas == 0 ? "Todos os testes passaram\n" : "Houve falhas\n");
        return falhas == 0 ? 0 : 1;
    }
}
