#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

#include "tsp/core/Ponto.hpp"

namespace tsp::core {
    /* Um cenário é o conjunto de cidades mais o que já se sabe dele de antemão.
    Só o círculo tem ótimo conhecido (o próprio contorno), por isso o optional:
    no uniforme não existe gap pra calcular */
    struct Cenario {
        std::string nome; // "uniforme" ou "circulo", vai pro resumo.txt
        std::vector<Ponto> pontos;
        std::optional<double> otimo; // distância do ciclo ótimo, quando conhecida
    };

    // numPontos sorteados uniformemente no quadrado [0, lado] x [0, lado]
    // mesma seed no rng -> mesmos pontos, então dá pra repetir a instância trocando só a seed do AG
    Cenario gerarUniforme(int numPontos, double lado, std::mt19937& rng);

    /* numPontos igualmente espaçados num círculo de raio 'raio' centrado em (raio, raio),
    ou seja, dentro da mesma caixa do uniforme com lado = 2 * raio (facilita comparar os gráficos).
    Os índices saem em ordem angular, mas isso não ajuda o AG: a população inicial é
    embaralhada e nenhum operador olha pro valor do índice, só pras distâncias */
    Cenario gerarCirculo(int numPontos, double raio);

    // Perímetro do polígono regular inscrito, 2·n·r·sin(π/n): é o ótimo do círculo,
    // porque qualquer outra ordem cruza o interior e fica mais longa
    double otimoCirculo(int numPontos, double raio);

    // Quanto a distância encontrada está acima do ótimo, em %: 100 · (distância - ótimo) / ótimo
    double gapPercentual(double distancia, double otimo);
}
