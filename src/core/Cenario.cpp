#include "tsp/core/Cenario.hpp"

#include <cmath>     // cos, sin
#include <stdexcept> // invalid_argument

namespace tsp::core {
    namespace {
        // M_PI não faz parte do padrão C++17, então define aqui
        constexpr double PI = 3.14159265358979323846;

        void validarNumCidades(int numCidades) {
            // com 0 ou 1 cidade não existe ciclo; o mínimo do enunciado (8) é checado na linha de comando
            if (numCidades < 2) throw std::invalid_argument("numCidades deve ser >= 2");
        }
    }

    Cenario gerarUniforme(int numCidades, double lado, std::mt19937& rng) {
        validarNumCidades(numCidades);
        if (lado <= 0) throw std::invalid_argument("lado deve ser > 0");

        std::uniform_real_distribution<double> distribuicao(0.0, lado);

        Cenario cenario;
        cenario.nome = "uniforme";
        cenario.pontos.reserve(numCidades);

        for (int i = 0; i < numCidades; ++i) {
            /* x e y sorteados em linhas separadas de propósito: numa chamada como f(dist(rng), dist(rng))
            o C++ não garante qual argumento é avaliado primeiro, e x e y poderiam trocar
            de compilador pra compilador, quebrando a reprodutibilidade da seed */
            double x = distribuicao(rng);
            double y = distribuicao(rng);
            cenario.pontos.push_back({x, y});
        }
        // otimoConhecido fica std::nullopt: não existe fórmula para o ótimo de pontos aleatórios
        return cenario;
    }

    Cenario gerarCirculo(int numCidades, double raio) {
        validarNumCidades(numCidades);
        if (raio <= 0) throw std::invalid_argument("raio deve ser > 0");

        Cenario cenario;
        cenario.nome = "circulo";
        cenario.pontos.reserve(numCidades);

        for (int i = 0; i < numCidades; ++i) {
            double angulo = 2.0 * PI * i / numCidades; // pontos igualmente espaçados
            cenario.pontos.push_back({raio * std::cos(angulo), raio * std::sin(angulo)});
        }
        cenario.otimoConhecido = otimoCirculo(numCidades, raio);
        return cenario;
    }

    double otimoCirculo(int numCidades, double raio) {
        return 2.0 * numCidades * raio * std::sin(PI / numCidades);
    }
}
