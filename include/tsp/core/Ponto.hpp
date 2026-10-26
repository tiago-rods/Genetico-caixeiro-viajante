#pragma once

namespace tsp::core {
    /* Uma cidade no plano. O AG só enxerga índices (o cromossomo é uma permutação),
    o Ponto é o que dá significado geométrico a cada índice: a cidade i é pontos[i] */
    struct Ponto {
        double x;
        double y;
    };
}
