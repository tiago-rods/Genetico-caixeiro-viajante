#pragma once
 
#include <random> 
#include <vector> 

#include "tsp/genetico/Individuo.hpp"

namespace tsp::genetico {
    /*factory de seleção
    isso é: dada a população inteira e a aptidao de cada um, (mesmo indice == mesmo individuo nos dois vetores)
    retorna UM individuo escolhido . AlgoritmoGenetico (pessoa C) so vai conhecer essa interface
    nunca o torneio direto, assim o laço evolutivo não precisa ser recompulado se a seleção mudar
    */
    class ISelecao {
    public: 
        virtual ~ISelecao() = default;
        
        /*const no final significa que o o selecionar não modifica o estado interno do objeto do torneio
        isso é, o k do torneio não muda a cada chamada*/
        virtual const Individuo& selecionar(const std::vector<Individuo>& populacao, const std::vector<double>& aptidoes,
            std::mt19937& rng) const = 0;

    };
}