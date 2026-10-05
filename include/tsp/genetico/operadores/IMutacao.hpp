#pragma once
#include <random> 
#include "tsp/genetico/Individuo.hpp"

namespace tsp::genetico {
    // Interface de mutação, diferente de cruzar(), aqui recebe-se um individuo (nao const)
    // e alteramos inplace ao invés de devolver um novo
    // Mutacao é uma pequena perturbação no que já existe, não algo novo

    class Imutacao {
    public:
        virtual ~Imutacao() = default;
        /*Lembrando que o const = 0, garante que não modifica o estado interno do objeto*/
        virtual void mutar(Individuo& individuo, std::mt19937& rng) const = 0;
    };
}