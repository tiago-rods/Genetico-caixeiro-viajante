#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

#include "Verificacao.hpp"
#include "tsp/core/Cenario.hpp"
#include "tsp/core/Distancia.hpp"

using namespace tsp;
using tsp::testes::verificar;

namespace {
    bool quaseIgual(double a, double b, double tolerancia = 1e-9) {
        return std::abs(a - b) <= tolerancia * std::max(1.0, std::abs(b));
    }

    std::vector<int> identidade(int n) {
        std::vector<int> rota(n);
        std::iota(rota.begin(), rota.end(), 0);
        return rota;
    }
}

int main() {
    // quadrado unitário: contorno mede 4; cruzando pelas diagonais mede 2 + 2·√2
    const std::vector<core::Ponto> quadrado{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const core::MatrizDistancias matrizQuadrado(quadrado);
    verificar(quaseIgual(core::distanciaRota({0, 1, 2, 3}, matrizQuadrado), 4.0), "quadrado unitario mede 4");
    verificar(quaseIgual(core::distanciaRota({0, 2, 1, 3}, matrizQuadrado), 2.0 + 2.0 * std::sqrt(2.0)),
              "quadrado cruzado mede 2 + 2*sqrt(2)");
    verificar(quaseIgual(core::distanciaRota({2, 3, 0, 1}, matrizQuadrado), 4.0),
              "rotacionar a rota nao muda a distancia (ciclo fechado)");
    verificar(quaseIgual(core::distanciaRota({3, 2, 1, 0}, matrizQuadrado), 4.0),
              "inverter o sentido nao muda a distancia");
    verificar(matrizQuadrado(1, 3) == matrizQuadrado(3, 1) && matrizQuadrado(2, 2) == 0.0,
              "matriz simetrica com diagonal zero");

    // ótimo do círculo pela fórmula confere com a soma das arestas na ordem angular
    for (int n : {8, 50, 1000}) {
        const core::Cenario circulo = core::gerarCirculo(n, 50.0);
        const core::MatrizDistancias matriz(circulo.pontos);
        verificar(circulo.otimo.has_value() &&
                      quaseIgual(*circulo.otimo, core::distanciaRota(identidade(n), matriz)),
                  "otimo do circulo = soma das arestas do poligono");
    }
    verificar(quaseIgual(core::otimoCirculo(100000, 1.0), 2.0 * 3.14159265358979323846, 1e-6),
              "otimo do circulo tende a 2*pi*r com muitos pontos");

    // uniforme: dentro do quadrado e reprodutível pela seed
    std::mt19937 rngA(7), rngB(7);
    const core::Cenario uniformeA = core::gerarUniforme(200, 100.0, rngA);
    const core::Cenario uniformeB = core::gerarUniforme(200, 100.0, rngB);
    bool dentro = true, iguais = true;
    for (std::size_t i = 0; i < uniformeA.pontos.size(); ++i) {
        const core::Ponto& p = uniformeA.pontos[i];
        dentro = dentro && p.x >= 0.0 && p.x <= 100.0 && p.y >= 0.0 && p.y <= 100.0;
        iguais = iguais && p.x == uniformeB.pontos[i].x && p.y == uniformeB.pontos[i].y;
    }
    verificar(dentro, "pontos do uniforme dentro de [0, lado]^2");
    verificar(iguais, "mesma seed gera os mesmos pontos");
    verificar(!uniformeA.otimo.has_value(), "uniforme nao tem otimo conhecido");

    // aptidão: rota mais curta tem aptidão maior
    verificar(quaseIgual(core::aptidao(4.0), 0.25), "aptidao = 1 / distancia");
    verificar(core::aptidao(10.0) > core::aptidao(20.0), "rota mais curta tem aptidao maior");

    verificar(quaseIgual(core::gapPercentual(110.0, 100.0), 10.0), "gap percentual");

    return tsp::testes::resultadoFinal();
}
