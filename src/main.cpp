// Executa o AG em um cenário, acompanha cada época no terminal e grava os arquivos para os gráficos.
// Exemplo: tsp --cenario circulo --n 50 --saida saidas/circulo

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>

#include "tsp/algoritmo/AlgoritmoGenetico.hpp"
#include "tsp/core/Cenario.hpp"
#include "tsp/core/Distancia.hpp"
#include "tsp/genetico/operadores/ERX.hpp"
#include "tsp/genetico/operadores/InversaoMutacao.hpp"
#include "tsp/genetico/operadores/Torneio.hpp"
#include "tsp/io/ArgParser.hpp"
#include "tsp/io/CsvWriter.hpp"

namespace {
    using namespace tsp;

    constexpr double kLadoUniforme = 100.0;
    constexpr double kRaioCirculo = 50.0; // o círculo cabe no mesmo quadrado [0, 100]² do uniforme
    constexpr int kMinimoPontos = 8;      // mínimo pedido no enunciado

    void imprimirAjuda() {
        const algoritmo::Parametros padrao;
        std::cout
            << "uso: tsp [opcoes]\n"
            << "  --cenario uniforme|circulo   (padrao: uniforme)\n"
            << "  --n N                        numero de pontos, >= " << kMinimoPontos << " (padrao: 50)\n"
            << "  --seed-pontos S              seed dos pontos do uniforme (padrao: 42)\n"
            << "  --seed S                     seed do AG (padrao: " << padrao.seed << ")\n"
            << "  --populacao P                (padrao: " << padrao.tamanhoPopulacao << ")\n"
            << "  --cruzamento T               taxa de cruzamento (padrao: " << padrao.taxaCruzamento << ")\n"
            << "  --mutacao T                  taxa de mutacao (padrao: " << padrao.taxaMutacao << ")\n"
            << "  --torneio K                  (padrao: " << padrao.tamanhoTorneio << ")\n"
            << "  --elite E                    (padrao: " << padrao.tamanhoElite << ")\n"
            << "  --geracoes G                 limite de geracoes (padrao: " << padrao.maxGeracoes << ")\n"
            << "  --estagnacao G               para apos G geracoes sem melhora (padrao: max("
            << padrao.maxEstagnacao << ", 3*n))\n"
            << "  --log-cada K                 imprime uma linha a cada K epocas (padrao: 10)\n"
            << "  --saida DIR                  pasta dos CSVs (padrao: saidas/<cenario>)\n";
    }

    core::Cenario criarCenario(const std::string& tipo, int numPontos, std::uint32_t seedPontos) {
        if (numPontos < kMinimoPontos) {
            throw std::invalid_argument("--n precisa ser pelo menos " + std::to_string(kMinimoPontos));
        }
        if (tipo == "uniforme") {
            std::mt19937 rngPontos(seedPontos);
            return core::gerarUniforme(numPontos, kLadoUniforme, rngPontos);
        }
        if (tipo == "circulo") {
            return core::gerarCirculo(numPontos, kRaioCirculo);
        }
        throw std::invalid_argument("--cenario precisa ser 'uniforme' ou 'circulo', recebeu '" + tipo + "'");
    }

    // "ger    40 | melhor   612.35 | media   700.12 | div   3.21 | gap  1.20% |   0.15 s *"
    // o '*' marca que a melhor rota melhorou desde a linha anterior
    void imprimirEpoca(const algoritmo::EstatisticaGeracao& e, const std::optional<double>& otimo, bool melhorou) {
        std::cout << "ger " << std::setw(5) << e.geracao
                  << " | melhor " << std::setw(9) << e.melhor
                  << " | media " << std::setw(9) << e.media
                  << " | div " << std::setw(6) << e.diversidade;
        if (otimo) {
            // a mesma rota ótima somada em outra ordem pode dar 1e-14 abaixo do ótimo; sem o max sairia "-0.00%"
            std::cout << " | gap " << std::setw(6) << std::max(0.0, core::gapPercentual(e.melhor, *otimo)) << '%';
        }
        std::cout << " | " << std::setw(7) << e.tempoMs / 1000.0 << " s" << (melhorou ? " *" : "") << '\n';
    }
}

int main(int argc, char* argv[]) {
    try {
        io::ArgParser args(argc, argv);
        if (args.tem("ajuda")) {
            imprimirAjuda();
            return 0;
        }

        const std::string tipoCenario = args.texto("cenario", "uniforme");
        const int numPontos = args.inteiro("n", 50);
        const auto seedPontos = static_cast<std::uint32_t>(args.inteiro("seed-pontos", 42));

        algoritmo::Parametros parametros;
        parametros.seed = static_cast<std::uint32_t>(args.inteiro("seed", static_cast<int>(parametros.seed)));
        parametros.tamanhoPopulacao = args.inteiro("populacao", parametros.tamanhoPopulacao);
        parametros.taxaCruzamento = args.real("cruzamento", parametros.taxaCruzamento);
        parametros.taxaMutacao = args.real("mutacao", parametros.taxaMutacao);
        parametros.tamanhoTorneio = args.inteiro("torneio", parametros.tamanhoTorneio);
        parametros.tamanhoElite = args.inteiro("elite", parametros.tamanhoElite);
        parametros.maxGeracoes = args.inteiro("geracoes", parametros.maxGeracoes);
        parametros.maxEstagnacao = args.inteiro("estagnacao", algoritmo::estagnacaoPadrao(numPontos));

        const int logCada = std::max(1, args.inteiro("log-cada", 10));
        const std::filesystem::path pastaSaida = args.texto("saida", "saidas/" + tipoCenario);

        if (auto desconhecidas = args.chavesNaoUsadas(); !desconhecidas.empty()) {
            throw std::invalid_argument("opcao desconhecida: " + desconhecidas.front());
        }
        algoritmo::validar(parametros);

        const core::Cenario cenario = criarCenario(tipoCenario, numPontos, seedPontos);
        const core::MatrizDistancias distancias(cenario.pontos);

        const genetico::Torneio selecao(static_cast<std::size_t>(parametros.tamanhoTorneio));
        const genetico::ERX cruzamento;
        const genetico::InversaoMutacao mutacao;
        const algoritmo::AlgoritmoGenetico ag(distancias, parametros, selecao, cruzamento, mutacao);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "cenario " << cenario.nome << ", " << numPontos << " pontos";
        if (cenario.otimo) std::cout << ", otimo " << *cenario.otimo;
        std::cout << "\npopulacao " << parametros.tamanhoPopulacao << ", mutacao " << parametros.taxaMutacao
                  << ", cruzamento " << parametros.taxaCruzamento << ", torneio " << parametros.tamanhoTorneio
                  << ", elite " << parametros.tamanhoElite << ", estagnacao " << parametros.maxEstagnacao
                  << ", seed " << parametros.seed << "\n\n";

        // acompanhamento por época; o histórico completo (toda época) vai pro historico.csv
        bool melhorouDesdeUltimaLinha = false;
        int ultimaImpressa = -1;
        const algoritmo::Resultado resultado = ag.executar(
            [&](const algoritmo::EstatisticaGeracao& e, bool melhorou) {
                melhorouDesdeUltimaLinha = melhorouDesdeUltimaLinha || melhorou;
                if (e.geracao % logCada == 0) {
                    imprimirEpoca(e, cenario.otimo, melhorouDesdeUltimaLinha);
                    melhorouDesdeUltimaLinha = false;
                    ultimaImpressa = e.geracao;
                }
            });
        if (ultimaImpressa != resultado.geracoes) {
            imprimirEpoca(resultado.historico.back(), cenario.otimo, melhorouDesdeUltimaLinha);
        }

        std::filesystem::create_directories(pastaSaida);
        io::gravarPontos(pastaSaida / "pontos.csv", cenario.pontos);
        io::gravarHistorico(pastaSaida / "historico.csv", resultado.historico);
        io::gravarRotas(pastaSaida / "rotas.csv", resultado.melhorias);
        io::gravarResumo(pastaSaida / "resumo.txt", cenario, seedPontos, parametros, resultado);

        std::cout << "\nmelhor distancia: " << resultado.melhorDistancia;
        if (cenario.otimo) {
            std::cout << " (otimo " << *cenario.otimo << ", gap "
                      << std::setprecision(4) << core::gapPercentual(resultado.melhorDistancia, *cenario.otimo)
                      << std::setprecision(2) << "%)";
        }
        std::cout << "\nparada: " << algoritmo::nomeMotivo(resultado.motivo) << " na geracao " << resultado.geracoes
                  << "\nultima melhora: geracao " << resultado.geracaoUltimaMelhora
                  << " (" << resultado.tempoUltimaMelhoraMs / 1000.0 << " s)"
                  << "\ntempo total: " << resultado.tempoTotalMs / 1000.0 << " s"
                  << "\narquivos em " << pastaSaida.string() << '\n';
    } catch (const std::exception& erro) {
        std::cerr << "erro: " << erro.what() << "\n(use --ajuda para ver as opcoes)\n";
        return 1;
    }
    return 0;
}
