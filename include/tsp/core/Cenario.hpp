#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "tsp/core/Ponto.hpp"

namespace tsp::core {
    /* Uma instância do problema: os pontos e, se existir, o comprimento da rota ótima.
    O índice no vetor de pontos é o id da cidade usado no cromossomo */
    struct Cenario {
        std::string nome;                     // "uniforme" ou "circulo"
        std::vector<Ponto> pontos;
        std::optional<double> otimoConhecido; // só o círculo tem; no uniforme fica std::nullopt
    };

    /* N pontos sorteados uniformemente em [0, lado] x [0, lado].
    Usa o rng recebido, então mesma seed -> mesmos pontos (experimento reproduzível) */
    Cenario gerarUniforme(int numCidades, double lado, std::mt19937& rng);

    /* N pontos igualmente espaçados num círculo de raio 'raio' centrado na origem.
    Saem em ordem angular, então a rota 0, 1, ..., N-1 é a ótima. Não precisa de rng:
    a população inicial do AG já é aleatória, isso não facilita nada pro algoritmo */
    Cenario gerarCirculo(int numCidades, double raio);

    /* Comprimento da rota ótima no círculo: N cordas iguais de ângulo 2*pi/N,
    cada uma medindo 2*R*sin(pi/N) -> total 2*N*R*sin(pi/N) */
    double otimoCirculo(int numCidades, double raio);
}
