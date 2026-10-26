#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

#include "Assercoes.hpp"
#include "genetico/VerificarPermutacao.hpp"
#include "tsp/core/Cenario.hpp"
#include "tsp/core/Distancia.hpp"
#include "tsp/genetico/Individuo.hpp"

using namespace tsp::core;
using tsp::testes::quaseIgual;
using tsp::testes::verificar;

namespace {
    // quadrado de lado 1: (0,0) (1,0) (1,1) (0,1)
    MatrizDistancias matrizQuadradoUnitario() {
        return calcularMatrizDistancias({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
    }

    template <typename Funcao>
    bool lancaInvalidArgument(Funcao funcao) {
        try {
            funcao();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }

    void testarDistanciaEuclidiana() {
        verificar(quaseIgual(distanciaEuclidiana({0, 0}, {3, 4}), 5.0), "distancia (0,0)-(3,4) = 5 (triangulo 3-4-5)");
        verificar(quaseIgual(distanciaEuclidiana({2, 7}, {2, 7}), 0.0), "distancia de um ponto a ele mesmo = 0");
    }

    void testarMatriz() {
        std::mt19937 rng(42);
        Cenario cenario = gerarUniforme(20, 100.0, rng);
        MatrizDistancias d = calcularMatrizDistancias(cenario.pontos);

        bool simetrica = true;
        bool diagonalZero = true;
        bool confereComEuclidiana = true;
        for (std::size_t i = 0; i < d.size(); ++i) {
            if (d[i][i] != 0.0) diagonalZero = false;
            for (std::size_t j = 0; j < d.size(); ++j) {
                if (d[i][j] != d[j][i]) simetrica = false;
                if (!quaseIgual(d[i][j], distanciaEuclidiana(cenario.pontos[i], cenario.pontos[j]))) {
                    confereComEuclidiana = false;
                }
            }
        }
        verificar(d.size() == 20 && d[0].size() == 20, "matriz tem N x N");
        verificar(simetrica, "matriz e simetrica");
        verificar(diagonalZero, "diagonal da matriz e zero");
        verificar(confereComEuclidiana, "cada celula confere com distanciaEuclidiana");
    }

    void testarDistanciaRota() {
        MatrizDistancias d = matrizQuadradoUnitario();

        verificar(quaseIgual(distanciaRota({0, 1, 2, 3}, d), 4.0), "quadrado unitario mede 4 (ciclo fechado)");
        verificar(quaseIgual(distanciaRota({0, 2, 1, 3}, d), 2.0 + 2.0 * std::sqrt(2.0)),
                  "rota cruzada do quadrado mede 2 + 2*sqrt(2)");

        // o mesmo ciclo começando em outra cidade ou percorrido ao contrário tem o mesmo comprimento;
        // só é verdade se a aresta de volta for contada
        verificar(quaseIgual(distanciaRota({2, 3, 0, 1}, d), 4.0), "rotacao da rota nao muda a distancia");
        verificar(quaseIgual(distanciaRota({3, 2, 1, 0}, d), 4.0), "rota invertida nao muda a distancia");
    }

    void testarCirculo() {
        Cenario cenario = gerarCirculo(8, 1.0);
        MatrizDistancias d = calcularMatrizDistancias(cenario.pontos);

        bool todosNoRaio = true;
        for (const Ponto& p : cenario.pontos) {
            if (!quaseIgual(std::hypot(p.x, p.y), 1.0)) todosNoRaio = false;
        }
        verificar(cenario.nome == "circulo" && cenario.pontos.size() == 8, "circulo gera N pontos");
        verificar(todosNoRaio, "todos os pontos do circulo estao a distancia R da origem");
        verificar(cenario.otimoConhecido.has_value(), "circulo tem otimo conhecido");

        std::vector<int> identidade = {0, 1, 2, 3, 4, 5, 6, 7};
        verificar(quaseIgual(distanciaRota(identidade, d), otimoCirculo(8, 1.0)),
                  "otimo do circulo (N=8) confere com a soma das arestas da rota em ordem angular");

        verificar(quaseIgual(otimoCirculo(4, 1.0), 4.0 * std::sqrt(2.0)), "otimo com N=4 e o quadrado inscrito, 4*sqrt(2)");
        verificar(std::fabs(otimoCirculo(100000, 1.0) - 2.0 * 3.14159265358979323846) < 1e-6,
                  "com N grande o otimo tende ao perimetro 2*pi*R");

        Cenario grande = gerarCirculo(500, 10.0);
        std::vector<int> identidadeGrande(500);
        for (int i = 0; i < 500; ++i) identidadeGrande[i] = i;
        verificar(quaseIgual(distanciaRota(identidadeGrande, calcularMatrizDistancias(grande.pontos)),
                             *grande.otimoConhecido, 1e-6),
                  "otimo confere tambem com N=500 e R=10 (cenario do bonus)");

        // integração com o bloco B: nenhuma rota aleatória (Individuo) pode ser menor que o ótimo
        std::mt19937 rng(7);
        bool nenhumaAbaixoDoOtimo = true;
        bool todasPermutacoesValidas = true;
        for (int k = 0; k < 1000; ++k) {
            tsp::genetico::Individuo rota = tsp::genetico::gerarIndividuoAleatorio(8, rng);
            if (!tsp::genetico::testes::ehPermutacaoValida(rota, 8)) todasPermutacoesValidas = false;
            if (distanciaRota(rota, d) < *cenario.otimoConhecido - 1e-9) nenhumaAbaixoDoOtimo = false;
        }
        verificar(todasPermutacoesValidas, "Individuo do bloco B e aceito por distanciaRota");
        verificar(nenhumaAbaixoDoOtimo, "1000 rotas aleatorias no circulo nunca ficam abaixo do otimo");
    }

    void testarUniforme() {
        std::mt19937 rngA(123);
        std::mt19937 rngB(123);
        std::mt19937 rngC(999);
        Cenario a = gerarUniforme(50, 100.0, rngA);
        Cenario b = gerarUniforme(50, 100.0, rngB);
        Cenario c = gerarUniforme(50, 100.0, rngC);

        bool iguais = true;
        bool dentroDoQuadrado = true;
        for (std::size_t i = 0; i < a.pontos.size(); ++i) {
            if (a.pontos[i].x != b.pontos[i].x || a.pontos[i].y != b.pontos[i].y) iguais = false;
            const Ponto& p = a.pontos[i];
            if (p.x < 0 || p.x > 100.0 || p.y < 0 || p.y > 100.0) dentroDoQuadrado = false;
        }
        verificar(a.nome == "uniforme" && a.pontos.size() == 50, "uniforme gera N pontos");
        verificar(iguais, "mesma seed -> mesmos pontos");
        verificar(a.pontos[0].x != c.pontos[0].x, "seeds diferentes -> pontos diferentes");
        verificar(dentroDoQuadrado, "todos os pontos dentro de [0, lado] x [0, lado]");
        verificar(!a.otimoConhecido.has_value(), "uniforme nao tem otimo conhecido");
    }

    void testarAptidao() {
        MatrizDistancias d = matrizQuadradoUnitario();
        double aptidaoOtima = aptidao(distanciaRota({0, 1, 2, 3}, d));
        double aptidaoCruzada = aptidao(distanciaRota({0, 2, 1, 3}, d));

        verificar(quaseIgual(aptidao(4.0), 0.25), "aptidao(4) = 0.25");
        verificar(aptidaoOtima > aptidaoCruzada, "rota mais curta tem aptidao maior (o Torneio maximiza)");
        verificar(lancaInvalidArgument([] { aptidao(0.0); }), "aptidao(0) lanca invalid_argument");
    }

    void testarEntradasInvalidas() {
        std::mt19937 rng(1);
        verificar(lancaInvalidArgument([&] { gerarUniforme(1, 100.0, rng); }), "uniforme com 1 cidade lanca erro");
        verificar(lancaInvalidArgument([&] { gerarUniforme(10, 0.0, rng); }), "uniforme com lado 0 lanca erro");
        verificar(lancaInvalidArgument([] { gerarCirculo(0, 1.0); }), "circulo com 0 cidades lanca erro");
        verificar(lancaInvalidArgument([] { gerarCirculo(10, -1.0); }), "circulo com raio negativo lanca erro");
    }
}

int main() {
    testarDistanciaEuclidiana();
    testarMatriz();
    testarDistanciaRota();
    testarCirculo();
    testarUniforme();
    testarAptidao();
    testarEntradasInvalidas();
    return tsp::testes::resultado();
}
