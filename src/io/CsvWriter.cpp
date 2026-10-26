#include "tsp/io/CsvWriter.hpp"

#include <iomanip>
#include <sstream>

namespace tsp::io {

    namespace {
        // Abre pra escrita com precisão suficiente pras distâncias e sem notação local
        // (o locale padrão "C" garante ponto decimal, que é o que o pandas/numpy esperam)
        void abrir(std::ofstream& arquivo, const std::filesystem::path& caminho) {
            arquivo.open(caminho);
            if (!arquivo) {
                throw std::runtime_error("nao foi possivel abrir '" + caminho.string() + "' para escrita");
            }
            arquivo << std::setprecision(12);
        }
    }

    CsvWriter::CsvWriter(const std::filesystem::path& caminho, const std::vector<std::string>& cabecalho)
        : colunas_(cabecalho.size()) {
        abrir(arquivo_, caminho);
        for (std::size_t i = 0; i < cabecalho.size(); ++i) {
            arquivo_ << (i == 0 ? "" : ",") << cabecalho[i];
        }
        arquivo_ << '\n';
    }

    std::string rotaComoTexto(const std::vector<int>& rota) {
        std::ostringstream texto;
        for (std::size_t i = 0; i < rota.size(); ++i) {
            texto << (i == 0 ? "" : " ") << rota[i];
        }
        return texto.str();
    }

    void gravarPontos(const std::filesystem::path& caminho, const std::vector<core::Ponto>& pontos) {
        CsvWriter csv(caminho, {"indice", "x", "y"});
        for (std::size_t i = 0; i < pontos.size(); ++i) {
            csv.linha(i, pontos[i].x, pontos[i].y);
        }
    }

    void gravarHistorico(const std::filesystem::path& caminho, const std::vector<algoritmo::EstatisticaGeracao>& historico) {
        CsvWriter csv(caminho, {"geracao", "melhor", "media", "pior", "desvio", "diversidade", "tempo_ms"});
        for (const auto& e : historico) {
            csv.linha(e.geracao, e.melhor, e.media, e.pior, e.desvioPadrao, e.diversidade, e.tempoMs);
        }
    }

    void gravarRotas(const std::filesystem::path& caminho, const std::vector<algoritmo::MelhoriaRota>& melhorias) {
        CsvWriter csv(caminho, {"geracao", "distancia", "tempo_ms", "rota"});
        for (const auto& m : melhorias) {
            csv.linha(m.geracao, m.distancia, m.tempoMs, rotaComoTexto(m.rota));
        }
    }

    void gravarResumo(const std::filesystem::path& caminho, const core::Cenario& cenario, std::uint32_t seedPontos,
                      const algoritmo::Parametros& p, const algoritmo::Resultado& r) {
        std::ofstream arquivo;
        abrir(arquivo, caminho);

        arquivo << "cenario: " << cenario.nome << '\n'
                << "pontos: " << cenario.pontos.size() << '\n'
                << "seed_pontos: " << seedPontos << '\n'
                << "seed: " << p.seed << '\n'
                << "populacao: " << p.tamanhoPopulacao << '\n'
                << "taxa_cruzamento: " << p.taxaCruzamento << '\n'
                << "taxa_mutacao: " << p.taxaMutacao << '\n'
                << "torneio: " << p.tamanhoTorneio << '\n'
                << "elite: " << p.tamanhoElite << '\n'
                << "max_geracoes: " << p.maxGeracoes << '\n'
                << "max_estagnacao: " << p.maxEstagnacao << '\n'
                << "melhor_distancia: " << r.melhorDistancia << '\n';
        if (cenario.otimo) {
            arquivo << "otimo: " << *cenario.otimo << '\n'
                    << "gap_percentual: " << core::gapPercentual(r.melhorDistancia, *cenario.otimo) << '\n';
        }
        arquivo << "geracoes: " << r.geracoes << '\n'
                << "motivo_parada: " << algoritmo::nomeMotivo(r.motivo) << '\n'
                << "geracao_ultima_melhora: " << r.geracaoUltimaMelhora << '\n'
                << "tempo_total_s: " << r.tempoTotalMs / 1000.0 << '\n'
                << "tempo_ate_ultima_melhora_s: " << r.tempoUltimaMelhoraMs / 1000.0 << '\n'
                << "melhor_rota: " << rotaComoTexto(r.melhorRota) << '\n';
    }
}
