#include "tsp/io/CsvWriter.hpp"

#include <fstream>
#include <iomanip>   // fixed, setprecision
#include <locale>
#include <stdexcept> // runtime_error

namespace tsp::io {
    namespace {
        /* Abre o arquivo já no formato combinado com o bloco D:
        locale clássico ("C") garante ponto decimal mesmo se alguém trocar o locale global,
        fixed + 6 casas evita notação científica (1e-05), que complicaria a leitura */
        std::ofstream abrir(const std::filesystem::path& arquivo) {
            if (arquivo.has_parent_path()) std::filesystem::create_directories(arquivo.parent_path());

            std::ofstream saida(arquivo);
            if (!saida.is_open()) throw std::runtime_error("nao foi possivel abrir " + arquivo.string());

            saida.imbue(std::locale::classic());
            saida << std::fixed << std::setprecision(6);
            return saida;
        }

        // ofstream não lança exceção sozinho: confere no fim se alguma escrita falhou (ex.: disco cheio)
        void conferir(const std::ofstream& saida, const std::filesystem::path& arquivo) {
            if (!saida) throw std::runtime_error("erro ao escrever " + arquivo.string());
        }
    }

    void escreverPontos(const std::filesystem::path& arquivo, const std::vector<core::Ponto>& pontos) {
        std::ofstream saida = abrir(arquivo);
        saida << "indice,x,y\n";
        for (std::size_t i = 0; i < pontos.size(); ++i) {
            saida << i << ',' << pontos[i].x << ',' << pontos[i].y << '\n';
        }
        conferir(saida, arquivo);
    }

    void escreverHistorico(const std::filesystem::path& arquivo, const std::vector<RegistroEpoca>& historico) {
        std::ofstream saida = abrir(arquivo);
        saida << "epoca,melhor,media,pior,tempo_ms\n";
        for (const RegistroEpoca& r : historico) {
            saida << r.epoca << ',' << r.melhorDistancia << ',' << r.mediaDistancia << ','
                  << r.piorDistancia << ',' << r.tempoMs << '\n';
        }
        conferir(saida, arquivo);
    }

    void escreverRotas(const std::filesystem::path& arquivo, const std::vector<SnapshotRota>& snapshots) {
        std::ofstream saida = abrir(arquivo);
        saida << "epoca,distancia,rota\n";
        for (const SnapshotRota& s : snapshots) {
            saida << s.epoca << ',' << s.distancia << ',';
            // cidades separadas por espaço, numa coluna só: vírgula aqui quebraria o CSV
            for (std::size_t i = 0; i < s.rota.size(); ++i) {
                if (i > 0) saida << ' ';
                saida << s.rota[i];
            }
            saida << '\n';
        }
        conferir(saida, arquivo);
    }

    void escreverResumo(const std::filesystem::path& arquivo, const Resumo& resumo) {
        std::ofstream saida = abrir(arquivo);
        // chave=valor: legível a olho e trivial de ler no Python (split no '=')
        saida << "cenario=" << resumo.cenario << '\n'
              << "num_cidades=" << resumo.numCidades << '\n'
              << "seed=" << resumo.seed << '\n'
              << "epocas=" << resumo.epocas << '\n'
              << "melhor_distancia=" << resumo.melhorDistancia << '\n';

        if (resumo.otimo) {
            // gap = quanto a melhor rota encontrada ficou acima do ótimo, em %
            double gap = (resumo.melhorDistancia - *resumo.otimo) / *resumo.otimo * 100.0;
            saida << "otimo=" << *resumo.otimo << '\n'
                  << "gap_percentual=" << gap << '\n';
        }

        saida << "tempo_total_ms=" << resumo.tempoTotalMs << '\n'
              << "epoca_ultima_melhora=" << resumo.epocaUltimaMelhora << '\n'
              << "tempo_ate_ultima_melhora_ms=" << resumo.tempoAteUltimaMelhoraMs << '\n';

        for (const auto& [chave, valor] : resumo.parametros) {
            saida << chave << '=' << valor << '\n';
        }
        conferir(saida, arquivo);
    }
}
