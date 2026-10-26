#include "tsp/core/Distancia.hpp"

#include <cmath>

namespace tsp::core {

    double distanciaEuclidiana(const Ponto& a, const Ponto& b) {
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    MatrizDistancias::MatrizDistancias(const std::vector<Ponto>& pontos)
        : n_(pontos.size()), distancias_(pontos.size() * pontos.size(), 0.0) {
        // simétrica: calcula só o triângulo de cima e espelha
        for (std::size_t i = 0; i < n_; ++i) {
            for (std::size_t j = i + 1; j < n_; ++j) {
                double d = distanciaEuclidiana(pontos[i], pontos[j]);
                distancias_[i * n_ + j] = d;
                distancias_[j * n_ + i] = d;
            }
        }
    }

    double distanciaRota(const std::vector<int>& rota, const MatrizDistancias& distancias) {
        if (rota.empty()) return 0.0;

        // começa pela aresta de volta (última -> primeira), que fecha o ciclo
        double total = distancias(rota.back(), rota.front());
        for (std::size_t i = 0; i + 1 < rota.size(); ++i) {
            total += distancias(rota[i], rota[i + 1]);
        }
        return total;
    }

    double aptidao(double distanciaTotal) {
        return 1.0 / distanciaTotal;
    }
}
