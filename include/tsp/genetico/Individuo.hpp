#pragma once

#include <vector>
#include <random>

namespace tsp::genetico {
    /* Cromossomo: uma rota é uma permutação dos indices das cidades
    isso é, se forem 5 cidades, a rota pode ser representada como {0, 1, 2, 3, 4}
    e coloquei um alias (using) pois vector<int> já tem tudo necessário*/
    using Individuo = std::vector<int>;
    
    /*Gera uma permutação aleatória de cidades 0 ... numCidades-1 
    se colocar a mesma seed terá o mesmo resultado*/
    Individuo gerarIndividuoAleatorio(int numCidades, std::mt19937& gerador);

}

