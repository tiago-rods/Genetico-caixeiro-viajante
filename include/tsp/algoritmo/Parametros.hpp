#pragma once

#include <cstdint>

namespace tsp::algoritmo {
    /* Todos os "botões" do AG num lugar só. Os padrões são os valores escolhidos a partir
    da varredura do experimentos.cpp; a justificativa de cada um está no README
    (População e critério de parada, Taxa de mutação) e na decisão 006.
    int em vez de size_t de propósito: um valor negativo vindo da linha de comando
    continua negativo e o validar() pega, em vez de virar um número gigante */
    struct Parametros {
        int tamanhoPopulacao = 400;
        double taxaCruzamento = 0.9; // prob. de um par de pais ser cruzado; senão o filho é cópia do pai1
        double taxaMutacao = 0.5;    // prob. de um filho sofrer uma inversão de trecho
        int tamanhoTorneio = 3;      // k do torneio
        int tamanhoElite = 2;        // melhores copiados intactos pra próxima geração
        int maxGeracoes = 20000;     // teto de segurança; na prática a estagnação para bem antes
        int maxEstagnacao = 300;     // para se a melhor rota passar tantas gerações sem melhorar
        std::uint32_t seed = 42;     // seed do AG (os pontos do cenário têm a seed deles, no main)
    };

    /* Limite de estagnação padrão pra n cidades: max(300, 3·n). Com mais cidades cada melhora
    fica mais rara (a inversão certa tem chance ~1/n²), e o limite que o círculo precisa pra não
    parar antes do ótimo cresceu com n nos experimentos. Quem sabe o n (main, experimentos) usa isso */
    int estagnacaoPadrao(int numCidades);

    // Lança std::invalid_argument dizendo qual campo está fora do intervalo
    void validar(const Parametros& parametros);
}
