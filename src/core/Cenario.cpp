#include "tsp/core/Cenario.hpp"

#include <cmath>
#include <stdexcept>

namespace tsp::core {

    namespace {
        constexpr double kPi = 3.14159265358979323846;

        // Ciclo só faz sentido com pelo menos 3 cidades (o mínimo de 8 do enunciado é checado no main)
        void exigirPontosSuficientes(int numPontos) {
            if (numPontos < 3) {
                throw std::invalid_argument("cenario precisa de pelo menos 3 pontos");
            }
        }
    }

    Cenario gerarUniforme(int numPontos, double lado, std::mt19937& rng) {
        exigirPontosSuficientes(numPontos);

        std::uniform_real_distribution<double> coordenada(0.0, lado);

        Cenario cenario;
        cenario.nome = "uniforme";
        cenario.pontos.reserve(numPontos);
        for (int i = 0; i < numPontos; ++i) {
            // x e y em duas linhas separadas: dentro de um mesmo construtor a ordem de avaliação
            // dos argumentos não é garantida, e a reprodutibilidade por seed dependeria do compilador
            double x = coordenada(rng);
            double y = coordenada(rng);
            cenario.pontos.push_back({x, y});
        }
        return cenario; // ótimo desconhecido, otimo fica vazio
    }

    Cenario gerarCirculo(int numPontos, double raio) {
        exigirPontosSuficientes(numPontos);

        Cenario cenario;
        cenario.nome = "circulo";
        cenario.pontos.reserve(numPontos);
        for (int i = 0; i < numPontos; ++i) {
            double angulo = 2.0 * kPi * i / numPontos;
            cenario.pontos.push_back({raio + raio * std::cos(angulo), raio + raio * std::sin(angulo)});
        }
        cenario.otimo = otimoCirculo(numPontos, raio);
        return cenario;
    }

    double otimoCirculo(int numPontos, double raio) {
        // cada aresta do polígono é a corda de um ângulo 2π/n: 2·r·sin(π/n), e são n arestas
        return 2.0 * numPontos * raio * std::sin(kPi / numPontos);
    }

    double gapPercentual(double distancia, double otimo) {
        return 100.0 * (distancia - otimo) / otimo;
    }
}
