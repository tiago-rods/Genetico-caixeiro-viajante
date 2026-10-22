#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "tsp/algoritmo/AlgoritmoGenetico.hpp"
#include "tsp/algoritmo/Parametros.hpp"
#include "tsp/core/Cenario.hpp"

namespace tsp::io {
    /* Escritor de CSV mínimo: cabeçalho no construtor, uma linha por chamada de linha().
    Não faz escape de vírgula nem de aspas porque só gravamos números e nomes simples;
    a rota vai como "3 0 2 1" (espaços), então também não precisa de aspas */
    class CsvWriter {
    public:
        CsvWriter(const std::filesystem::path& caminho, const std::vector<std::string>& cabecalho);

        // variádico: linha(geracao, melhor, media) grava "12,345.6,400.1", qualquer tipo com operator<<
        template <typename... Valores>
        void linha(const Valores&... valores) {
            if (sizeof...(valores) != colunas_) {
                throw std::logic_error("linha com numero de colunas diferente do cabecalho");
            }
            const char* separador = "";
            ((arquivo_ << separador << valores, separador = ","), ...);
            arquivo_ << '\n';
        }

    private:
        std::ofstream arquivo_;
        std::size_t colunas_;
    };

    // "3 0 2 1": formato da rota dentro de uma célula de CSV ou do resumo.txt
    std::string rotaComoTexto(const std::vector<int>& rota);

    /* Arquivos de saída de uma execução (formato combinado com quem plota, pessoa D):
    pontos.csv    -> indice,x,y
    historico.csv -> geracao,melhor,media,pior,desvio,diversidade,tempo_ms   (uma linha por época)
    rotas.csv     -> geracao,distancia,tempo_ms,rota                          (uma linha por melhoria)
    resumo.txt    -> "chave: valor" por linha, parâmetros e resultado final */
    void gravarPontos(const std::filesystem::path& caminho, const std::vector<core::Ponto>& pontos);
    void gravarHistorico(const std::filesystem::path& caminho, const std::vector<algoritmo::EstatisticaGeracao>& historico);
    void gravarRotas(const std::filesystem::path& caminho, const std::vector<algoritmo::MelhoriaRota>& melhorias);
    void gravarResumo(const std::filesystem::path& caminho, const core::Cenario& cenario, std::uint32_t seedPontos,
                      const algoritmo::Parametros& parametros, const algoritmo::Resultado& resultado);
}
