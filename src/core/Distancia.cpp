#include "tsp/core/Distancia.hpp"

#include <cmath>     // hypot
#include <stdexcept> // invalid_argument

namespace tsp::core {
    double distanciaEuclidiana(const Ponto& a, const Ponto& b) {
        // hypot calcula sqrt(dx² + dy²) sem risco de overflow no quadrado
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    MatrizDistancias calcularMatrizDistancias(const std::vector<Ponto>& pontos) {
        std::size_t numCidades = pontos.size();
        MatrizDistancias distancias(numCidades, std::vector<double>(numCidades, 0.0)); // diagonal já fica 0

        // a distância é simétrica: calcula só i < j e espelha, metade das contas
        for (std::size_t i = 0; i < numCidades; ++i) {
            for (std::size_t j = i + 1; j < numCidades; ++j) {
                double d = distanciaEuclidiana(pontos[i], pontos[j]);
                distancias[i][j] = d;
                distancias[j][i] = d;
            }
        }
        return distancias;
    }

    double distanciaRota(const std::vector<int>& rota, const MatrizDistancias& distancias) {
        std::size_t numCidades = rota.size();
        double total = 0.0;

        for (std::size_t i = 0; i < numCidades; ++i) {
            // (i + 1) % numCidades: na última cidade volta pra primeira, fechando o ciclo
            // mesmo truque circular que o ERX usa para montar a tabela de vizinhos
            int origem = rota[i];
            int destino = rota[(i + 1) % numCidades];
            total += distancias[origem][destino];
        }
        return total;
    }

    double aptidao(double distanciaTotal) {
        // distância 0 só acontece se todas as cidades coincidem; 1/0 não é uma aptidão válida
        if (distanciaTotal <= 0) throw std::invalid_argument("distanciaTotal deve ser > 0");
        return 1.0 / distanciaTotal;
    }
}
