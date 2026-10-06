#pragma once

#include <random>
#include "tsp/genetico/Individuo.hpp"

namespace tsp::genetico {
    // interface de cruzamento: recebe dois pais, devolve UM filho, permutação valida, 
    // A lógica de como combinar (ERX) fica dentro de quem implementar essa interface
    class ICruzamento {
    public:
        virtual ~ICruzamento() = default;
        
        /*com const = 0, garante que não modifica o estado interno do objeto*/
        virtual Individuo cruzar(const Individuo& pai1, const Individuo& pai2, std::mt19937& rng) const = 0;

    };
}