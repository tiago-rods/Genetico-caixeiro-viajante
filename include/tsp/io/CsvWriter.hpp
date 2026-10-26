#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "tsp/core/Ponto.hpp"

namespace tsp::io {
    /* Formato combinado com o bloco D (viz/): separador vírgula, decimal com PONTO
    (locale "C", independente do Windows estar em pt-BR), cabeçalho na 1a linha, 6 casas decimais */

    // uma linha do historico.csv; preenchido pelo bloco C a cada época
    struct RegistroEpoca {
        int epoca = 0;
        double melhorDistancia = 0.0;
        double mediaDistancia = 0.0;
        double piorDistancia = 0.0;
        double tempoMs = 0.0; // tempo acumulado desde o início da execução
    };

    // uma linha do rotas.csv; o bloco C decide de quantas em quantas épocas guardar
    struct SnapshotRota {
        int epoca = 0;
        double distancia = 0.0;
        std::vector<int> rota; // NÃO repete a primeira cidade no fim; quem plota fecha o ciclo
    };

    // conteúdo do resumo.txt
    struct Resumo {
        std::string cenario;
        int numCidades = 0;
        unsigned seed = 0;
        int epocas = 0;
        double melhorDistancia = 0.0;
        std::optional<double> otimo; // se existir (círculo), também escreve o gap percentual
        double tempoTotalMs = 0.0;
        int epocaUltimaMelhora = 0;
        double tempoAteUltimaMelhoraMs = 0.0;
        // parâmetros do AG em texto livre (populacao=100, ...): o bloco A não precisa conhecer os Parametros do C
        std::vector<std::pair<std::string, std::string>> parametros;
    };

    // Todas criam a pasta do arquivo se ela não existir
    // e lançam std::runtime_error se não conseguirem escrever
    void escreverPontos(const std::filesystem::path& arquivo, const std::vector<core::Ponto>& pontos);
    void escreverHistorico(const std::filesystem::path& arquivo, const std::vector<RegistroEpoca>& historico);
    void escreverRotas(const std::filesystem::path& arquivo, const std::vector<SnapshotRota>& snapshots);
    void escreverResumo(const std::filesystem::path& arquivo, const Resumo& resumo);
}
