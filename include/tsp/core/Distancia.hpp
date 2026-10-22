#pragma once

#include <cstddef>
#include <vector>

#include "tsp/core/Ponto.hpp"

namespace tsp::core {
    double distanciaEuclidiana(const Ponto& a, const Ponto& b);

    /* Matriz n x n com todas as distâncias pré-calculadas.
    O AG avalia (população x gerações) rotas de n arestas cada; calcular a raiz toda vez
    seria refazer a mesma conta milhões de vezes. Fica num vector só (linha * n + coluna)
    pra ser contígua na memória. Custo: 8·n² bytes, ~32 MB pra 2000 pontos */
    class MatrizDistancias {
    public:
        explicit MatrizDistancias(const std::vector<Ponto>& pontos);

        double operator()(int i, int j) const {
            return distancias_[static_cast<std::size_t>(i) * n_ + static_cast<std::size_t>(j)];
        }

        int tamanho() const { return static_cast<int>(n_); }

    private:
        std::size_t n_;
        std::vector<double> distancias_;
    };

    /* Comprimento do ciclo fechado: soma rota[i] -> rota[i+1] e a volta rota[n-1] -> rota[0].
    Recebe std::vector<int> (que é o genetico::Individuo) pra core não depender de genetico */
    double distanciaRota(const std::vector<int>& rota, const MatrizDistancias& distancias);

    /* Função de aptidão: 1 / distância total do ciclo.
    O problema é de minimização, mas seleção trabalha com "maior é melhor"; inverter
    mantém a ordem (rota mais curta -> aptidão maior) e é sempre positiva, porque
    distância > 0 com pelo menos 2 pontos distintos. Ver README, seção Função de aptidão */
    double aptidao(double distanciaTotal);
}
