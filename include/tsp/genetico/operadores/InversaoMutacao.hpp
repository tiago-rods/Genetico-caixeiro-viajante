#pragma once

#include "tsp/genetico/operadores/IMutacao.hpp"

namespace tsp::genetico {
   /* Mutacao por inversao de trecho, sorteia dois indices, i e j, sendo i < j
   dentro da rota e inverte a ordem das cidades entre eles. Como só reordcena elementos
   o resultado é sempre uma permutacao valida */
    class InversaoMutacao : public Imutacao {
        public:
            void mutar(Individuo& individuo, std::mt19937& rng) const override; 
    };
}