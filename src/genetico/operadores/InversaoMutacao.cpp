#include "tsp/genetico/operadores/InversaoMutacao.hpp"
#include <algorithm> // para std::reverse e swap
#include <random> // para std::mt19937 e std::uniform_int_distribution

namespace tsp::genetico {

    void InversaoMutacao::mutar(Individuo& individuo, std::mt19937& rng) const {
        // Rota de 0 ou 1 cidades não pode ser invertida
        if (individuo.size() <= 1) return;

        // Distruibuicao sobre os indices validos [0, size-1]. size_t pois e o tipo que vector::size() retorna
        std::uniform_int_distribution<std::size_t> distribuicaoIndice(0, individuo.size() - 1);
        std::uniform_int_distribution<std::size_t> distribuicaoDeslocamento(1, individuo.size() - 1);

        std::size_t i = distribuicaoIndice(rng);
        std::size_t j = (i + distribuicaoDeslocamento(rng)) % individuo.size(); // sorteia outro indice, diferente de i, mas ainda dentro do vetor
        if (i > j) std::swap(i, j); // garante que i < j
        // pensei em colocar um caso de j++ se for igual a i, mas ai poderia estourar o limite do vetor.

        

        //reverse trabalha com intervalo [1°, ultimo), por isso j+1
        std::reverse(individuo.begin() + i, individuo.begin() + j + 1);
    }
}