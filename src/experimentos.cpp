/* Experimentos que embasam a escolha dos parâmetros (README: População e critério de parada, Taxa de mutação).

Dois modos, ambos com várias seeds do AG sobre a MESMA instância de pontos:
  varredura   grade população x taxa de mutação, cada execução com o critério de parada normal
              -> varredura.csv (uma linha por execução) e resumo_varredura.csv (médias por combinação)
  estagnacao  roda sem parada antecipada até --geracoes-longas e, a partir das gerações em que a melhor
              rota melhorou, calcula exatamente onde cada limite de estagnação teria parado e quanto
              de distância se perderia por parar ali (o rng não depende da parada, então a execução
              com limite S é idêntica à longa até o ponto de parada)
              -> estagnacao.csv e resumo_estagnacao.csv

Exemplo: experimentos --modo varredura --n 50 --seeds 10 --populacoes 50,100,200 --mutacoes 0,0.3,1

Os tempos medidos aqui são de execuções simultâneas (uma por thread) e servem só pra comparar
configurações entre si; tempo "de verdade" (bônus) é medido com o tsp, que roda sozinho. */

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

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

    // mesmas dimensões do main, pra que os números daqui valham pras execuções de lá
    constexpr double kLadoUniforme = 100.0;
    constexpr double kRaioCirculo = 50.0;
    constexpr double kToleranciaOtimo = 1e-6; // gap (%) abaixo disso conta como ótimo atingido

    // Instância compartilhada (só leitura) por todas as execuções de um cenário
    struct Instancia {
        core::Cenario cenario;
        core::MatrizDistancias distancias;
    };

    Instancia criarInstancia(const std::string& tipo, int numPontos, std::uint32_t seedPontos) {
        if (tipo == "uniforme") {
            std::mt19937 rngPontos(seedPontos);
            core::Cenario cenario = core::gerarUniforme(numPontos, kLadoUniforme, rngPontos);
            core::MatrizDistancias distancias(cenario.pontos);
            return {std::move(cenario), std::move(distancias)};
        }
        if (tipo == "circulo") {
            core::Cenario cenario = core::gerarCirculo(numPontos, kRaioCirculo);
            core::MatrizDistancias distancias(cenario.pontos);
            return {std::move(cenario), std::move(distancias)};
        }
        throw std::invalid_argument("cenario desconhecido: " + tipo);
    }

    struct Execucao {
        std::size_t instancia;
        algoritmo::Parametros parametros;
        algoritmo::Resultado resultado;
    };

    /* Roda as execuções em paralelo, cada thread pegando a próxima da fila.
    Cada execução tem sua seed e seu rng, e operadores/matriz são só leitura, então o resultado
    de cada uma é o mesmo que seria rodando sozinha. As mais pesadas (população maior) vão primeiro
    pra nenhuma thread ficar com a última execução grande sozinha no final */
    void rodarTodas(std::vector<Execucao>& execucoes, const std::vector<Instancia>& instancias, int numThreads) {
        const genetico::ERX cruzamento;
        const genetico::InversaoMutacao mutacao;

        std::vector<std::size_t> ordem(execucoes.size());
        for (std::size_t i = 0; i < ordem.size(); ++i) ordem[i] = i;
        std::stable_sort(ordem.begin(), ordem.end(), [&](std::size_t a, std::size_t b) {
            return execucoes[a].parametros.tamanhoPopulacao > execucoes[b].parametros.tamanhoPopulacao;
        });

        std::atomic<std::size_t> proxima{0};
        std::atomic<std::size_t> concluidas{0};
        std::mutex mutexProgresso;

        auto trabalhador = [&]() {
            for (std::size_t k = proxima++; k < ordem.size(); k = proxima++) {
                Execucao& e = execucoes[ordem[k]];
                const genetico::Torneio selecao(static_cast<std::size_t>(e.parametros.tamanhoTorneio));
                const algoritmo::AlgoritmoGenetico ag(instancias[e.instancia].distancias, e.parametros,
                                                      selecao, cruzamento, mutacao);
                e.resultado = ag.executar();

                // progresso a cada ~1%, senão o log redirecionado vira uma linha por execução
                const std::size_t feitas = ++concluidas;
                if (feitas % std::max<std::size_t>(1, execucoes.size() / 100) == 0 || feitas == execucoes.size()) {
                    std::lock_guard<std::mutex> trava(mutexProgresso);
                    std::cerr << "\r  " << feitas << "/" << execucoes.size() << " execucoes" << std::flush;
                }
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) threads.emplace_back(trabalhador);
        for (std::thread& t : threads) t.join();
        std::cerr << '\n';
    }

    /* Qualidade relativa: gap ao ótimo no círculo; no uniforme, que não tem ótimo conhecido,
    gap à melhor distância encontrada em todas as execuções do experimento */
    std::vector<double> referencias(const std::vector<Execucao>& execucoes, const std::vector<Instancia>& instancias) {
        std::vector<double> referencia(instancias.size(), std::numeric_limits<double>::infinity());
        for (const Execucao& e : execucoes) {
            referencia[e.instancia] = std::min(referencia[e.instancia], e.resultado.melhorDistancia);
        }
        for (std::size_t i = 0; i < instancias.size(); ++i) {
            if (instancias[i].cenario.otimo) referencia[i] = *instancias[i].cenario.otimo;
        }
        return referencia;
    }

    struct Estatistica {
        double media = 0.0, desvio = 0.0, minimo = 0.0, maximo = 0.0;
    };

    Estatistica resumir(const std::vector<double>& valores) {
        Estatistica s;
        if (valores.empty()) return s;
        s.minimo = *std::min_element(valores.begin(), valores.end());
        s.maximo = *std::max_element(valores.begin(), valores.end());
        for (double v : valores) s.media += v;
        s.media /= static_cast<double>(valores.size());
        for (double v : valores) s.desvio += (v - s.media) * (v - s.media);
        s.desvio = std::sqrt(s.desvio / static_cast<double>(valores.size()));
        return s;
    }

    // ---------------------------------------------------------------- varredura

    void modoVarredura(const io::ArgParser& args, const std::vector<Instancia>& instancias,
                       const algoritmo::Parametros& base, int numSeeds, int numThreads,
                       const std::filesystem::path& pasta) {
        const std::vector<int> populacoes = args.listaInteiros("populacoes", {25, 50, 100, 200, 400});
        const std::vector<double> mutacoes = args.listaReais("mutacoes", {0.0, 0.05, 0.1, 0.2, 0.3, 0.5, 0.8, 1.0});

        std::vector<Execucao> execucoes;
        for (std::size_t i = 0; i < instancias.size(); ++i) {
            for (int populacao : populacoes) {
                for (double mutacao : mutacoes) {
                    for (int seed = 1; seed <= numSeeds; ++seed) {
                        algoritmo::Parametros p = base;
                        p.tamanhoPopulacao = populacao;
                        p.taxaMutacao = mutacao;
                        p.seed = static_cast<std::uint32_t>(seed);
                        algoritmo::validar(p);
                        execucoes.push_back({i, p, {}});
                    }
                }
            }
        }

        std::cerr << "varredura: " << populacoes.size() << " populacoes x " << mutacoes.size() << " mutacoes x "
                  << numSeeds << " seeds x " << instancias.size() << " cenarios\n";
        rodarTodas(execucoes, instancias, numThreads);
        const std::vector<double> referencia = referencias(execucoes, instancias);

        io::CsvWriter csv(pasta / "varredura.csv",
                          {"cenario", "n", "populacao", "taxa_mutacao", "seed", "melhor_distancia", "referencia",
                           "gap_pct", "geracoes", "geracao_ultima_melhora", "motivo", "tempo_ms",
                           "tempo_ultima_melhora_ms", "diversidade_final"});
        // chave (instância, população, mutação) -> gaps, gerações, tempos e diversidade das seeds
        struct Grupo {
            std::vector<double> gaps, geracoes, ultimaMelhora, tempos, diversidades;
        };
        std::map<std::tuple<std::size_t, int, double>, Grupo> grupos;

        for (const Execucao& e : execucoes) {
            const algoritmo::Resultado& r = e.resultado;
            const core::Cenario& cenario = instancias[e.instancia].cenario;
            // a referência é o ótimo ou o mínimo das execuções, então gap < 0 só pode ser ruído
            // de ponto flutuante (a rota ótima somada em outra ordem); o max evita "-0.000%"
            const double gap = std::max(0.0, core::gapPercentual(r.melhorDistancia, referencia[e.instancia]));
            csv.linha(cenario.nome, cenario.pontos.size(), e.parametros.tamanhoPopulacao, e.parametros.taxaMutacao,
                      e.parametros.seed, r.melhorDistancia, referencia[e.instancia], gap, r.geracoes,
                      r.geracaoUltimaMelhora, algoritmo::nomeMotivo(r.motivo), r.tempoTotalMs,
                      r.tempoUltimaMelhoraMs, r.historico.back().diversidade);

            Grupo& g = grupos[{e.instancia, e.parametros.tamanhoPopulacao, e.parametros.taxaMutacao}];
            g.gaps.push_back(gap);
            g.geracoes.push_back(r.geracoes);
            g.ultimaMelhora.push_back(r.geracaoUltimaMelhora);
            g.tempos.push_back(r.tempoTotalMs);
            g.diversidades.push_back(r.historico.back().diversidade);
        }

        io::CsvWriter resumo(pasta / "resumo_varredura.csv",
                             {"cenario", "populacao", "taxa_mutacao", "execucoes", "gap_medio_pct", "gap_desvio_pct",
                              "gap_min_pct", "gap_max_pct", "referencias_atingidas", "geracoes_media",
                              "ultima_melhora_media", "tempo_medio_ms", "diversidade_final_media"});
        std::cout << "\nVARREDURA (gap: ao otimo no circulo, a melhor distancia encontrada no uniforme)\n";
        std::size_t instanciaAnterior = instancias.size();
        for (const auto& [chave, g] : grupos) {
            const auto& [instancia, populacao, mutacao] = chave;
            const Estatistica gap = resumir(g.gaps);
            const auto atingidas = std::count_if(g.gaps.begin(), g.gaps.end(),
                                                 [](double v) { return v < kToleranciaOtimo; });
            const double geracoes = resumir(g.geracoes).media;
            const double ultimaMelhora = resumir(g.ultimaMelhora).media;
            const double tempo = resumir(g.tempos).media;
            const double diversidade = resumir(g.diversidades).media;

            resumo.linha(instancias[instancia].cenario.nome, populacao, mutacao, g.gaps.size(), gap.media, gap.desvio,
                         gap.minimo, gap.maximo, atingidas, geracoes, ultimaMelhora, tempo, diversidade);

            if (instancia != instanciaAnterior) {
                std::cout << "\n" << instancias[instancia].cenario.nome << " (" << instancias[instancia].cenario.pontos.size()
                          << " pontos, referencia " << referencia[instancia] << ")\n"
                          << "   pop   mut | gap medio   desvio      max | ref. atingida | geracoes ult.melhora"
                             " | tempo s | div final\n";
                instanciaAnterior = instancia;
            }
            std::cout << std::setw(6) << populacao << std::setw(6) << std::setprecision(2) << mutacao
                      << std::setprecision(3) << " | " << std::setw(8) << gap.media << "% " << std::setw(8)
                      << gap.desvio << std::setw(9) << gap.maximo << " | " << std::setw(6) << atingidas << "/"
                      << std::setw(2) << g.gaps.size() << "      | " << std::setw(8) << geracoes << std::setw(12)
                      << ultimaMelhora << " | " << std::setw(7) << tempo / 1000.0 << " | " << std::setw(9)
                      << diversidade << '\n';
        }
    }

    // ---------------------------------------------------------------- estagnação

    struct Parada {
        int geracao;
        double distancia;
        double tempoMs;
    };

    /* Onde uma execução com limite de estagnação 'limite' teria parado, dado o histórico de melhorias
    de uma execução sem parada antecipada. Depois da melhoria i o contador chega a 'limite' na geração
    g_i + limite; ela para ali se a próxima melhoria vier depois disso (ou se não houver próxima e
    g_i + limite ainda couber no teto). Senão chega no teto de gerações */
    Parada simularParada(const algoritmo::Resultado& longa, int limite) {
        const auto& melhorias = longa.melhorias;
        for (std::size_t i = 0; i < melhorias.size(); ++i) {
            const int proxima = i + 1 < melhorias.size() ? melhorias[i + 1].geracao : longa.geracoes + 1;
            const int parada = melhorias[i].geracao + limite;
            if (parada < proxima) {
                return {parada, melhorias[i].distancia, longa.historico[static_cast<std::size_t>(parada)].tempoMs};
            }
        }
        return {longa.geracoes, longa.melhorDistancia, longa.tempoTotalMs};
    }

    void modoEstagnacao(const io::ArgParser& args, const std::vector<Instancia>& instancias,
                        const algoritmo::Parametros& base, int numSeeds, int numThreads,
                        const std::filesystem::path& pasta) {
        const std::vector<int> limites = args.listaInteiros("estagnacoes", {25, 50, 100, 200, 300, 500, 1000, 2000});
        const int geracoesLongas = args.inteiro("geracoes-longas", 10000);

        std::vector<Execucao> execucoes;
        for (std::size_t i = 0; i < instancias.size(); ++i) {
            for (int seed = 1; seed <= numSeeds; ++seed) {
                algoritmo::Parametros p = base;
                p.maxGeracoes = geracoesLongas;
                p.maxEstagnacao = geracoesLongas; // nunca para antes do teto
                p.seed = static_cast<std::uint32_t>(seed);
                algoritmo::validar(p);
                execucoes.push_back({i, p, {}});
            }
        }

        std::cerr << "estagnacao: " << numSeeds << " seeds x " << instancias.size() << " cenarios, "
                  << geracoesLongas << " geracoes cada\n";
        rodarTodas(execucoes, instancias, numThreads);

        io::CsvWriter csv(pasta / "estagnacao.csv",
                          {"cenario", "n", "populacao", "taxa_mutacao", "seed", "estagnacao", "geracao_parada",
                           "distancia_parada", "distancia_longa", "perda_pct", "tempo_parada_ms", "tempo_longa_ms"});
        std::map<std::pair<std::size_t, int>, std::vector<std::pair<double, double>>> perdas; // (perda %, gerações)

        for (const Execucao& e : execucoes) {
            const core::Cenario& cenario = instancias[e.instancia].cenario;
            for (int limite : limites) {
                const Parada parada = simularParada(e.resultado, limite);
                const double perda = std::max(0.0, core::gapPercentual(parada.distancia, e.resultado.melhorDistancia));
                csv.linha(cenario.nome, cenario.pontos.size(), e.parametros.tamanhoPopulacao, e.parametros.taxaMutacao,
                          e.parametros.seed, limite, parada.geracao, parada.distancia, e.resultado.melhorDistancia,
                          perda, parada.tempoMs, e.resultado.tempoTotalMs);
                perdas[{e.instancia, limite}].push_back({perda, static_cast<double>(parada.geracao)});
            }
        }

        io::CsvWriter resumo(pasta / "resumo_estagnacao.csv",
                             {"cenario", "estagnacao", "execucoes", "perda_media_pct", "perda_max_pct",
                              "sem_perda", "geracao_parada_media"});
        std::cout << "\nESTAGNACAO (perda = quanto a rota ao parar fica acima da melhor rota em "
                  << geracoesLongas << " geracoes)\n";
        std::size_t instanciaAnterior = instancias.size();
        for (const auto& [chave, valores] : perdas) {
            const auto& [instancia, limite] = chave;
            std::vector<double> perdasPct, geracoes;
            for (const auto& [perda, geracao] : valores) {
                perdasPct.push_back(perda);
                geracoes.push_back(geracao);
            }
            const Estatistica perda = resumir(perdasPct);
            const auto semPerda = std::count_if(perdasPct.begin(), perdasPct.end(),
                                                [](double v) { return v < kToleranciaOtimo; });
            const double geracaoMedia = resumir(geracoes).media;
            resumo.linha(instancias[instancia].cenario.nome, limite, valores.size(), perda.media, perda.maximo,
                         semPerda, geracaoMedia);

            if (instancia != instanciaAnterior) {
                std::cout << "\n" << instancias[instancia].cenario.nome << " (" << instancias[instancia].cenario.pontos.size()
                          << " pontos)\n  limite | perda media   max | sem perda | geracao de parada media\n";
                instanciaAnterior = instancia;
            }
            std::cout << std::setw(8) << limite << " | " << std::setw(9) << perda.media << "% " << std::setw(6)
                      << perda.maximo << " | " << std::setw(5) << semPerda << "/" << std::setw(2) << valores.size()
                      << "   | " << std::setw(10) << geracaoMedia << '\n';
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        io::ArgParser args(argc, argv);
        const std::string modo = args.texto("modo", "ambos");
        const int numPontos = args.inteiro("n", 50);
        const std::vector<std::string> cenarios = args.listaTextos("cenarios", {"uniforme", "circulo"});
        const auto seedPontos = static_cast<std::uint32_t>(args.inteiro("seed-pontos", 42));
        const int numSeeds = args.inteiro("seeds", 10);
        int numThreads = args.inteiro("threads", 0);
        if (numThreads <= 0) numThreads = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
        const std::filesystem::path pasta = args.texto("saida", "saidas/experimentos");

        // parâmetros que não estão sendo variados no modo escolhido
        algoritmo::Parametros base;
        base.tamanhoPopulacao = args.inteiro("populacao", base.tamanhoPopulacao);
        base.taxaMutacao = args.real("mutacao", base.taxaMutacao);
        base.taxaCruzamento = args.real("cruzamento", base.taxaCruzamento);
        base.tamanhoTorneio = args.inteiro("torneio", base.tamanhoTorneio);
        base.tamanhoElite = args.inteiro("elite", base.tamanhoElite);
        base.maxGeracoes = args.inteiro("geracoes", base.maxGeracoes);
        base.maxEstagnacao = args.inteiro("estagnacao", algoritmo::estagnacaoPadrao(numPontos));

        const bool rodarVarredura = modo == "varredura" || modo == "ambos";
        const bool rodarEstagnacao = modo == "estagnacao" || modo == "ambos";
        if (!rodarVarredura && !rodarEstagnacao) {
            throw std::invalid_argument("--modo precisa ser varredura, estagnacao ou ambos");
        }
        // lê as opções específicas de cada modo antes de checar as desconhecidas
        if (rodarVarredura) {
            args.listaInteiros("populacoes", {});
            args.listaReais("mutacoes", {});
        }
        if (rodarEstagnacao) {
            args.listaInteiros("estagnacoes", {});
            args.inteiro("geracoes-longas", 0);
        }
        if (auto desconhecidas = args.chavesNaoUsadas(); !desconhecidas.empty()) {
            throw std::invalid_argument("opcao desconhecida: " + desconhecidas.front());
        }
        if (numSeeds < 1) throw std::invalid_argument("--seeds precisa ser pelo menos 1");

        std::vector<Instancia> instancias;
        for (const std::string& tipo : cenarios) instancias.push_back(criarInstancia(tipo, numPontos, seedPontos));

        std::filesystem::create_directories(pasta);
        std::cout << std::fixed << std::setprecision(3);
        std::cerr << numThreads << " threads, " << numPontos << " pontos\n";

        if (rodarVarredura) modoVarredura(args, instancias, base, numSeeds, numThreads, pasta);
        if (rodarEstagnacao) modoEstagnacao(args, instancias, base, numSeeds, numThreads, pasta);

        std::cout << "\narquivos em " << pasta.string() << '\n';
    } catch (const std::exception& erro) {
        std::cerr << "erro: " << erro.what() << '\n';
        return 1;
    }
    return 0;
}
