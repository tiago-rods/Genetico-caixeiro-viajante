#pragma once

#include "tsp/genetico/operadores/ICruzamento.hpp"

namespace tsp::genetico {

    // Crossover por Recombinação de Arestas (Edge Recombination Crossover - ERX)
    // Referência: Whitley, Starkweather & Fuquay (1989).
    /*
    Ideia: Preserva ao máximo as "arestas" (cidaeds vizinhas) que já existem nos pais
    montando o filho através de uma caminhada gulosa guiada por uma tabela de vizinhos
    cosntruida a partir dos dois pais
    */

    class ERX : public ICruzamento {
    public: 
        Individuo cruzar(const Individuo& pai1, const Individuo& pai2, std::mt19937& rng) const override;
    };
}