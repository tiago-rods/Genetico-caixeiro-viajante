#pragma once

#include <vector>

#include "tsp/genetico/Individuo.hpp"

namespace tsp::genetico::testes {

// Confere se 'individuo' e' uma permutacao valida de 0..numCidades-1:
// tamanho certo, sem repeticao, sem valor fora do intervalo.
// O(n) no tempo e no espaco - mais rapido que ordenar e comparar (O(n log n)),
// e cobre os 3 jeitos de um crossover/mutacao "quebrar": tamanho errado,
// cidade repetida, cidade fora do intervalo [0, numCidades).
    inline bool ehPermutacaoValida(const Individuo& individuo, int numCidades) { // inline para evitar problemas de multiplas definicoes em testes diferentes
        if (static_cast<int>(individuo.size()) != numCidades) {
            return false;
        }

        std::vector<bool> visto(numCidades, false);
        for (int cidade : individuo) {
            if (cidade < 0 || cidade >= numCidades || visto[cidade]) {
                return false;
            }
            visto[cidade] = true;
        }

        return true;
    }

}