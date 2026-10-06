#pragma once

#include "tsp/genetico/operadores/ISelecao.hpp"

namespace tsp::genetico {
    /* Seleção por torneio sorteia k individuos da populacao (com reposição, ou seja, o mesmo indice pode sair mais de uma vez(ver se isso é desejado))
    */    
   class Torneio : public ISelecao {
    public: 
        explicit Torneio(std::size_t tamanhoTorneio); //evita conversão implícita para Torneio, ou seja, Torneio t = 5; não compila, precisa ser Torneio t(5);


        const Individuo& selecionar(const std::vector<Individuo>& populacao, const std::vector<double>& aptidoes,
            std::mt19937& rng) const override;

    private:
        std::size_t tamanhoTorneio_; // é o k do torneio, ou seja, quantos individuos serão sorteados para disputar a seleção
   };
}