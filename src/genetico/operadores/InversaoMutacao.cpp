#include "tsp/genetico/operadores/InversaoMutacao.hpp"
#include <algorithm> // para std::reverse e swap
#include <random> // para std::mt19937 e std::uniform_int_distribution

namespace tsp::genetico {

    void InversaoMutacao::mutar(Individuo& individuo, std::mt19937& rng) const {
        // Rota de 0 ou 1 cidades não pode ser invertida
        if (individuo.size() <= 1) return;

        // Distruibuicao sobre os indices validos [0, size-1]. size_t pois e o tipo que vector::size() retorna
        std::uniform_int_distribution<size_t> distribuicao(0, individuo.size() - 1);

        std::size_t i = distribuicao(rng);
        std::size_t j = distribuicao(rng);
        if (i > j) std::swap(i, j); // garante que i < j

        //reverse trabalha com intervalo [1°, ultimo), por isso j+1
        std::reverse(individuo.begin() + i, individuo.begin() + j + 1);
    }
}