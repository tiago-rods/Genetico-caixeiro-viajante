#pragma once

namespace tsp::core {
    // Uma cidade no plano. Só guarda as coordenadas; o índice da cidade é a posição dela
    // no vetor de pontos do cenário, que é o mesmo índice usado no cromossomo
    struct Ponto {
        double x;
        double y;
    };
}
