#include "tsp/genetico/operadores/Torneio.hpp"

namespace tsp::genetico {
    Torneio::Torneio(std::size_t tamanhoTorneio)
        : tamanhoTorneio_(tamanhoTorneio) {}

    const Individuo& Torneio::selecionar(const std::vector<Individuo>& populacao, const std::vector<double>& aptidoes, std::mt19937& rng) const {
        std::uniform_int_distribution<std::size_t> distribuicao(0, populacao.size() - 1);  

        //sorteia o primeiro candidato como o melhor até o momento
        std::size_t melhorIndice = distribuicao(rng);

        //sorteia os outros (tamanhoTorneio_ - 1) e guarda a maior aptidão
        for (std::size_t i = 1; i < tamanhoTorneio_; ++i) {
            std::size_t canditato = distribuicao(rng);
            if (aptidoes[canditato] > aptidoes[melhorIndice]) {
                melhorIndice = canditato;
            }
        }

        return populacao[melhorIndice];
    };
    
}