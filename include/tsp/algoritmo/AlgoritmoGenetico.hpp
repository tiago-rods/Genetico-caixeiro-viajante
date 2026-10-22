#pragma once

#include <functional>
#include <vector>

#include "tsp/algoritmo/Parametros.hpp"
#include "tsp/core/Distancia.hpp"
#include "tsp/genetico/Individuo.hpp"
#include "tsp/genetico/operadores/ICruzamento.hpp"
#include "tsp/genetico/operadores/IMutacao.hpp"
#include "tsp/genetico/operadores/ISelecao.hpp"

namespace tsp::algoritmo {

    // Fotografia da população no fim de uma época (geração). Vira uma linha do historico.csv
    struct EstatisticaGeracao {
        int geracao; // 0 é a população inicial, antes de qualquer cruzamento
        double melhor; // menor distância da população
        double media;
        double pior;
        double desvioPadrao;
        /* arestas distintas presentes na população inteira / n (arestas de uma rota).
        Vale 1 quando todo mundo é a mesma rota (rotacionada ou ao contrário conta como igual)
        e cresce conforme a população tem rotas diferentes. É o que mostra o ERX
        "colapsando" a população e o quanto a mutação segura isso */
        double diversidade;
        double tempoMs; // tempo desde o início da execução
    };

    /* Melhor rota no momento em que ela melhorou. Com elitismo a melhor rota não muda entre
    duas melhorias, então essa lista basta pra reconstruir a melhor solução de qualquer época:
    é a última melhoria com geracao <= época. Guardar a rota de toda época repetiria a mesma rota
    milhares de vezes (rotas.csv com 500 pontos x 5000 gerações) */
    struct MelhoriaRota {
        int geracao;
        double distancia;
        double tempoMs;
        genetico::Individuo rota;
    };

    enum class MotivoParada { LimiteGeracoes, Estagnacao };

    const char* nomeMotivo(MotivoParada motivo); // "limite_geracoes" / "estagnacao", pro resumo e CSVs

    struct Resultado {
        genetico::Individuo melhorRota;
        double melhorDistancia = 0.0;
        int geracoes = 0; // última geração executada
        MotivoParada motivo = MotivoParada::LimiteGeracoes;
        double tempoTotalMs = 0.0;
        // "convergência" = última vez que a melhor rota melhorou; o que vem depois é só
        // o critério de parada esperando a estagnação se confirmar
        int geracaoUltimaMelhora = 0;
        double tempoUltimaMelhoraMs = 0.0;
        std::vector<EstatisticaGeracao> historico; // uma por época, inclusive a 0
        std::vector<MelhoriaRota> melhorias;       // a primeira é sempre a da geração 0
    };

    /* Laço evolutivo. Só conhece as interfaces de seleção/cruzamento/mutação (decisão 001),
    então trocar ERX por outro cruzamento é só injetar outra implementação.
    Guarda referências: a matriz e os operadores precisam viver mais que o AG
    (no main eles são variáveis locais declaradas antes dele) */
    class AlgoritmoGenetico {
    public:
        // chamado no fim de cada época; 'melhorou' diz se a melhor rota mudou nessa época
        using Observador = std::function<void(const EstatisticaGeracao&, bool melhorou)>;

        AlgoritmoGenetico(const core::MatrizDistancias& distancias, const Parametros& parametros,
                          const genetico::ISelecao& selecao, const genetico::ICruzamento& cruzamento,
                          const genetico::IMutacao& mutacao);

        /* const: o rng nasce da seed dentro de executar(), então chamar duas vezes
        dá exatamente o mesmo resultado (e dá pra rodar seeds diferentes em paralelo
        com AGs diferentes sobre a mesma matriz) */
        Resultado executar(const Observador& observador = {}) const;

    private:
        const core::MatrizDistancias& distancias_;
        Parametros parametros_;
        const genetico::ISelecao& selecao_;
        const genetico::ICruzamento& cruzamento_;
        const genetico::IMutacao& mutacao_;
    };
}
