#pragma once

#include <vector>

#include "tsp/core/Ponto.hpp"

namespace tsp::core {
    // distancias[i][j] = distância euclidiana entre a cidade i e a cidade j
    using MatrizDistancias = std::vector<std::vector<double>>;

    double distanciaEuclidiana(const Ponto& a, const Ponto& b);

    /* Pré-calcula todas as distâncias uma única vez, antes do AG.
    Custa O(N^2) de memória, mas cada avaliação de rota vira só leitura na matriz,
    sem sqrt, e o AG avalia rotas milhões de vezes */
    MatrizDistancias calcularMatrizDistancias(const std::vector<Ponto>& pontos);

    /* Comprimento do ciclo FECHADO: soma também a aresta de volta rota[N-1] -> rota[0].
    Recebe std::vector<int> (que é o mesmo tipo de genetico::Individuo, um alias)
    para core não depender de genetico, a dependência fica só no sentido A -> todos */
    double distanciaRota(const std::vector<int>& rota, const MatrizDistancias& distancias);

    /* Aptidão = 1 / distância. O Torneio maximiza a aptidão e o problema minimiza a distância,
    então precisa de uma função decrescente. Recebe a distância já calculada para quem chama
    (bloco C) poder usar o mesmo valor no histórico sem somar a rota de novo */
    double aptidao(double distanciaTotal);
}
