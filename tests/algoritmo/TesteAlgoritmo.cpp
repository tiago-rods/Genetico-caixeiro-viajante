#include <cmath>
#include <random>
#include <stdexcept>

#include "Verificacao.hpp"
#include "genetico/VerificarPermutacao.hpp"
#include "tsp/algoritmo/AlgoritmoGenetico.hpp"
#include "tsp/core/Cenario.hpp"
#include "tsp/core/Distancia.hpp"
#include "tsp/genetico/operadores/ERX.hpp"
#include "tsp/genetico/operadores/InversaoMutacao.hpp"
#include "tsp/genetico/operadores/Torneio.hpp"

using namespace tsp;
using tsp::testes::verificar;

namespace {
    const genetico::Torneio kSelecao(3);
    const genetico::ERX kCruzamento;
    const genetico::InversaoMutacao kMutacao;

    algoritmo::Resultado rodar(const core::MatrizDistancias& distancias, const algoritmo::Parametros& p) {
        return algoritmo::AlgoritmoGenetico(distancias, p, kSelecao, kCruzamento, kMutacao).executar();
    }

    bool parametrosRejeitados(const algoritmo::Parametros& p) {
        try {
            algoritmo::validar(p);
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }
}

int main() {
    std::mt19937 rngPontos(1);
    const core::Cenario uniforme = core::gerarUniforme(30, 100.0, rngPontos);
    const core::MatrizDistancias distanciasUniforme(uniforme.pontos);

    // com elitismo a melhor distância nunca piora, em nenhuma época e pra nenhuma seed
    // (mutação alta de propósito: é o caso que mais "estraga" filhos)
    bool nuncaPiora = true;
    for (std::uint32_t seed = 1; seed <= 5; ++seed) {
        algoritmo::Parametros p;
        p.tamanhoPopulacao = 40;
        p.taxaMutacao = 0.8;
        p.maxGeracoes = 300;
        p.seed = seed;
        const algoritmo::Resultado r = rodar(distanciasUniforme, p);
        for (std::size_t g = 1; g < r.historico.size(); ++g) {
            nuncaPiora = nuncaPiora && r.historico[g].melhor <= r.historico[g - 1].melhor;
        }
    }
    verificar(nuncaPiora, "com elitismo o melhor nunca piora");

    // círculo de 8 pontos: espaço pequeno (7!/2 = 2520 ciclos), o AG tem que achar o ótimo
    const core::Cenario circulo8 = core::gerarCirculo(8, 50.0);
    const core::MatrizDistancias distanciasCirculo8(circulo8.pontos);
    bool achouOtimo = true;
    for (std::uint32_t seed = 1; seed <= 10; ++seed) {
        algoritmo::Parametros p;
        p.seed = seed;
        const algoritmo::Resultado r = rodar(distanciasCirculo8, p);
        achouOtimo = achouOtimo && core::gapPercentual(r.melhorDistancia, *circulo8.otimo) < 1e-9;
    }
    verificar(achouOtimo, "circulo de 8 pontos atinge o otimo (10 seeds)");

    // resultado coerente: rota válida e distância batendo com a rota
    algoritmo::Parametros padrao;
    padrao.maxGeracoes = 200;
    const algoritmo::Resultado r1 = rodar(distanciasUniforme, padrao);
    verificar(genetico::testes::ehPermutacaoValida(r1.melhorRota, 30), "melhor rota e permutacao valida");
    verificar(std::abs(core::distanciaRota(r1.melhorRota, distanciasUniforme) - r1.melhorDistancia) < 1e-9,
              "melhorDistancia confere com a melhor rota");
    verificar(r1.historico.size() == static_cast<std::size_t>(r1.geracoes) + 1, "uma estatistica por epoca, inclusive a 0");
    verificar(r1.melhorias.front().geracao == 0 && r1.melhorias.back().distancia == r1.melhorDistancia,
              "melhorias comecam na geracao 0 e terminam na melhor rota");

    // reprodutível: mesma seed, mesmo resultado
    const algoritmo::Resultado r2 = rodar(distanciasUniforme, padrao);
    verificar(r1.melhorRota == r2.melhorRota && r1.geracoes == r2.geracoes, "mesma seed, mesmo resultado");

    // critérios de parada
    algoritmo::Parametros curto;
    curto.maxGeracoes = 5;
    curto.maxEstagnacao = 1000;
    const algoritmo::Resultado porLimite = rodar(distanciasUniforme, curto);
    verificar(porLimite.motivo == algoritmo::MotivoParada::LimiteGeracoes && porLimite.geracoes == 5,
              "para pelo limite de geracoes");

    algoritmo::Parametros estagna;
    estagna.maxEstagnacao = 20;
    const algoritmo::Resultado porEstagnacao = rodar(distanciasCirculo8, estagna);
    verificar(porEstagnacao.motivo == algoritmo::MotivoParada::Estagnacao &&
                  porEstagnacao.geracoes == porEstagnacao.geracaoUltimaMelhora + 20,
              "para por estagnacao exatamente 20 geracoes depois da ultima melhora");

    // validação dos parâmetros
    algoritmo::Parametros ruim;
    ruim.taxaMutacao = 1.5;
    verificar(parametrosRejeitados(ruim), "rejeita taxa de mutacao fora de [0, 1]");
    ruim = {};
    ruim.tamanhoElite = ruim.tamanhoPopulacao;
    verificar(parametrosRejeitados(ruim), "rejeita elite do tamanho da populacao");
    ruim = {};
    ruim.tamanhoPopulacao = -5;
    verificar(parametrosRejeitados(ruim), "rejeita populacao negativa");
    verificar(!parametrosRejeitados(algoritmo::Parametros{}), "aceita os parametros padrao");

    return tsp::testes::resultadoFinal();
}
