#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "Verificacao.hpp"
#include "genetico/VerificarPermutacao.hpp"
#include "tsp/core/Cenario.hpp"
#include "tsp/core/Distancia.hpp"
#include "tsp/genetico/Individuo.hpp"

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

    template <typename Funcao>
    bool lanca(Funcao funcao) {
        try {
            funcao();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
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

    // ---- casos portados da branch pessoa-A ----

    // distância euclidiana direto, sem passar pela matriz
    verificar(quaseIgual(core::distanciaEuclidiana({0, 0}, {3, 4}), 5.0), "distancia (0,0)-(3,4) = 5 (triangulo 3-4-5)");
    verificar(core::distanciaEuclidiana({2, 7}, {2, 7}) == 0.0, "distancia de um ponto a ele mesmo = 0");

    // matriz inteira (não só um par): um erro no espelhamento deixaria metade das arestas valendo 0
    {
        std::mt19937 rng(42);
        const core::Cenario cenario = core::gerarUniforme(20, 100.0, rng);
        const core::MatrizDistancias m(cenario.pontos);
        bool simetrica = true, diagonalZero = true, confere = true;
        for (int i = 0; i < m.tamanho(); ++i) {
            diagonalZero = diagonalZero && m(i, i) == 0.0;
            for (int j = 0; j < m.tamanho(); ++j) {
                simetrica = simetrica && m(i, j) == m(j, i);
                confere = confere && quaseIgual(m(i, j), core::distanciaEuclidiana(cenario.pontos[i], cenario.pontos[j]));
            }
        }
        verificar(m.tamanho() == 20, "matriz tem tamanho n");
        verificar(simetrica && diagonalZero, "matriz inteira simetrica com diagonal zero (20 pontos)");
        verificar(confere, "cada celula da matriz confere com distanciaEuclidiana");
    }

    // círculo: geometria dos pontos e ótimo contra valores conhecidos
    {
        const double raio = 50.0;
        const core::Cenario circulo = core::gerarCirculo(8, raio);
        bool todosNoRaio = true;
        for (const core::Ponto& p : circulo.pontos) {
            todosNoRaio = todosNoRaio && quaseIgual(std::hypot(p.x - raio, p.y - raio), raio); // centro em (r, r)
        }
        verificar(circulo.nome == "circulo" && circulo.pontos.size() == 8, "circulo gera n pontos com o nome certo");
        verificar(todosNoRaio, "todos os pontos do circulo a distancia r do centro (r, r)");
        verificar(quaseIgual(core::otimoCirculo(4, 1.0), 4.0 * std::sqrt(2.0)), "otimo com n=4 e o quadrado inscrito, 4*sqrt(2)");

        // integração com o bloco B: Individuo entra direto em distanciaRota, e nenhuma rota fica abaixo do ótimo
        const core::MatrizDistancias matriz(circulo.pontos);
        std::mt19937 rng(7);
        bool permutacoesValidas = true, nenhumaAbaixo = true;
        for (int k = 0; k < 1000; ++k) {
            const genetico::Individuo rota = genetico::gerarIndividuoAleatorio(8, rng);
            permutacoesValidas = permutacoesValidas && genetico::testes::ehPermutacaoValida(rota, 8);
            nenhumaAbaixo = nenhumaAbaixo && core::distanciaRota(rota, matriz) >= *circulo.otimo - 1e-9;
        }
        verificar(permutacoesValidas, "Individuo do bloco B e aceito por distanciaRota");
        verificar(nenhumaAbaixo, "1000 rotas aleatorias no circulo nunca ficam abaixo do otimo");
    }

    // uniforme: tamanho, nome e a seed realmente controla os pontos
    {
        std::mt19937 rngA(123), rngC(999);
        const core::Cenario a = core::gerarUniforme(50, 100.0, rngA);
        const core::Cenario c = core::gerarUniforme(50, 100.0, rngC);
        verificar(a.nome == "uniforme" && a.pontos.size() == 50, "uniforme gera n pontos com o nome certo");
        verificar(a.pontos[0].x != c.pontos[0].x, "seeds diferentes geram pontos diferentes");
    }

    // entradas inválidas: o mínimo da implementação é 3 pontos
    {
        std::mt19937 rng(1);
        verificar(lanca([&] { core::gerarUniforme(2, 100.0, rng); }), "uniforme com menos de 3 pontos lanca invalid_argument");
        verificar(lanca([] { core::gerarCirculo(0, 1.0); }), "circulo com 0 pontos lanca invalid_argument");
    }

    return tsp::testes::resultadoFinal();
}
