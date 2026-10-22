#include "tsp/algoritmo/AlgoritmoGenetico.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <utility>

namespace tsp::algoritmo {

    namespace {
        using Relogio = std::chrono::steady_clock;

        double msDesde(Relogio::time_point inicio) {
            return std::chrono::duration<double, std::milli>(Relogio::now() - inicio).count();
        }

        /* Tolerância relativa pra dizer que a melhor distância melhorou. A mesma rota começando
        em outra cidade (ou no sentido contrário) soma as mesmas arestas em outra ordem e pode
        diferir no último bit; sem tolerância isso contaria como melhora e zeraria a estagnação à toa */
        constexpr double kToleranciaMelhora = 1e-9;

        /* Conta arestas distintas na população (ver EstatisticaGeracao::diversidade).
        Em vez de limpar um vetor n x n a cada geração, cada aresta guarda a "marca" da última
        medição em que apareceu: aresta nova = marca diferente da atual. Fica O(P·n) por geração,
        o mesmo custo de avaliar a população */
        class MedidorDiversidade {
        public:
            explicit MedidorDiversidade(int numCidades)
                : n_(static_cast<std::size_t>(numCidades)), marcas_(n_ * n_, 0) {}

            double medir(const std::vector<genetico::Individuo>& populacao) {
                ++marcaAtual_;
                std::size_t distintas = 0;
                for (const genetico::Individuo& rota : populacao) {
                    for (std::size_t i = 0; i < rota.size(); ++i) {
                        auto a = static_cast<std::size_t>(rota[i]);
                        auto b = static_cast<std::size_t>(rota[(i + 1) % rota.size()]);
                        if (a > b) std::swap(a, b); // aresta sem sentido: (a, b) == (b, a)
                        unsigned& marca = marcas_[a * n_ + b];
                        if (marca != marcaAtual_) {
                            marca = marcaAtual_;
                            ++distintas;
                        }
                    }
                }
                return static_cast<double>(distintas) / static_cast<double>(n_);
            }

        private:
            std::size_t n_;
            std::vector<unsigned> marcas_;
            unsigned marcaAtual_ = 0;
        };
    }

    const char* nomeMotivo(MotivoParada motivo) {
        switch (motivo) {
            case MotivoParada::LimiteGeracoes: return "limite_geracoes";
            case MotivoParada::Estagnacao: return "estagnacao";
        }
        return "desconhecido";
    }

    AlgoritmoGenetico::AlgoritmoGenetico(const core::MatrizDistancias& distancias, const Parametros& parametros,
                                         const genetico::ISelecao& selecao, const genetico::ICruzamento& cruzamento,
                                         const genetico::IMutacao& mutacao)
        : distancias_(distancias), parametros_(parametros),
          selecao_(selecao), cruzamento_(cruzamento), mutacao_(mutacao) {
        validar(parametros_);
    }

    Resultado AlgoritmoGenetico::executar(const Observador& observador) const {
        const auto inicio = Relogio::now();
        const Parametros& p = parametros_;
        const int numCidades = distancias_.tamanho();
        const auto tamanhoPopulacao = static_cast<std::size_t>(p.tamanhoPopulacao);
        const auto tamanhoElite = static_cast<std::size_t>(p.tamanhoElite);

        std::mt19937 rng(p.seed);
        std::uniform_real_distribution<double> sorteio(0.0, 1.0);

        // população inicial: permutações uniformemente aleatórias, sem nenhuma heurística,
        // pra que o resultado venha só da evolução
        std::vector<genetico::Individuo> populacao;
        populacao.reserve(tamanhoPopulacao);
        for (std::size_t i = 0; i < tamanhoPopulacao; ++i) {
            populacao.push_back(genetico::gerarIndividuoAleatorio(numCidades, rng));
        }

        // mesmo índice = mesmo indivíduo nos três vetores (é o contrato da ISelecao)
        std::vector<double> distancias(tamanhoPopulacao);
        std::vector<double> aptidoes(tamanhoPopulacao);
        auto avaliar = [&]() {
            for (std::size_t i = 0; i < tamanhoPopulacao; ++i) {
                distancias[i] = core::distanciaRota(populacao[i], distancias_);
                aptidoes[i] = core::aptidao(distancias[i]);
            }
        };

        Resultado resultado;
        MedidorDiversidade medidorDiversidade(numCidades);

        // fecha a época: estatísticas, melhor rota e observador. Devolve se a melhor rota melhorou
        auto registrarEpoca = [&](int geracao) {
            EstatisticaGeracao estatistica{};
            estatistica.geracao = geracao;

            auto [itMenor, itMaior] = std::minmax_element(distancias.begin(), distancias.end());
            estatistica.melhor = *itMenor;
            estatistica.pior = *itMaior;
            estatistica.media = std::accumulate(distancias.begin(), distancias.end(), 0.0) / tamanhoPopulacao;

            double somaQuadrados = 0.0;
            for (double d : distancias) somaQuadrados += (d - estatistica.media) * (d - estatistica.media);
            estatistica.desvioPadrao = std::sqrt(somaQuadrados / tamanhoPopulacao);

            estatistica.diversidade = medidorDiversidade.medir(populacao);
            estatistica.tempoMs = msDesde(inicio);

            bool melhorou = resultado.melhorias.empty() ||
                            estatistica.melhor < resultado.melhorDistancia * (1.0 - kToleranciaMelhora);
            if (melhorou) {
                auto indiceMelhor = static_cast<std::size_t>(itMenor - distancias.begin());
                resultado.melhorRota = populacao[indiceMelhor];
                resultado.melhorDistancia = estatistica.melhor;
                resultado.melhorias.push_back({geracao, estatistica.melhor, estatistica.tempoMs, resultado.melhorRota});
            }

            resultado.historico.push_back(estatistica);
            if (observador) observador(estatistica, melhorou);
            return melhorou;
        };

        avaliar();
        registrarEpoca(0);

        std::vector<genetico::Individuo> proximaGeracao;
        proximaGeracao.reserve(tamanhoPopulacao);
        std::vector<std::size_t> ordem(tamanhoPopulacao);

        int geracao = 0;
        int geracoesSemMelhora = 0;
        resultado.motivo = MotivoParada::LimiteGeracoes;

        while (geracao < p.maxGeracoes) {
            ++geracao;
            proximaGeracao.clear();

            // elitismo: os 'tamanhoElite' melhores passam intactos (sem mutação), então a melhor
            // rota nunca piora de uma geração pra outra. partial_sort porque só o topo importa
            std::iota(ordem.begin(), ordem.end(), std::size_t{0});
            std::partial_sort(ordem.begin(), ordem.begin() + static_cast<std::ptrdiff_t>(tamanhoElite), ordem.end(),
                              [&](std::size_t a, std::size_t b) { return distancias[a] < distancias[b]; });
            for (std::size_t i = 0; i < tamanhoElite; ++i) {
                proximaGeracao.push_back(populacao[ordem[i]]);
            }

            // o resto da população: seleção -> cruzamento -> mutação, um filho por par de pais
            while (proximaGeracao.size() < tamanhoPopulacao) {
                const genetico::Individuo& pai1 = selecao_.selecionar(populacao, aptidoes, rng);
                const genetico::Individuo& pai2 = selecao_.selecionar(populacao, aptidoes, rng);

                genetico::Individuo filho = sorteio(rng) < p.taxaCruzamento
                                                ? cruzamento_.cruzar(pai1, pai2, rng)
                                                : pai1;
                if (sorteio(rng) < p.taxaMutacao) {
                    mutacao_.mutar(filho, rng);
                }
                proximaGeracao.push_back(std::move(filho));
            }

            // troca em vez de copiar: a geração antiga vira o buffer da próxima
            populacao.swap(proximaGeracao);
            avaliar();

            if (registrarEpoca(geracao)) {
                geracoesSemMelhora = 0;
            } else if (++geracoesSemMelhora >= p.maxEstagnacao) {
                resultado.motivo = MotivoParada::Estagnacao;
                break;
            }
        }

        resultado.geracoes = geracao;
        resultado.tempoTotalMs = msDesde(inicio);
        resultado.geracaoUltimaMelhora = resultado.melhorias.back().geracao;
        resultado.tempoUltimaMelhoraMs = resultado.melhorias.back().tempoMs;
        return resultado;
    }
}
