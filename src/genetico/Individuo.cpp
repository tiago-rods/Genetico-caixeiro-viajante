#include "tsp/genetico/Individuo.hpp"
#include <algorithm> // util para o shuffle
#include <numeric> // util para iota

namespace tsp::genetico {
    Individuo gerarIndividuoAleatorio(int numCidades, std::mt19937& rng){
        Individuo individuo(numCidades); // lembrando que Individuo é um alias para std::vector<int>
        std::iota(individuo.begin(), individuo.end(), 0); // iota preenche com 0, 1, 2, ..., numCidades-1 a rota em ordem ainda não embaralhada

        std::shuffle(individuo.begin(), individuo.end(), rng); // embaralha inplace usando o rng recebido
        /*Por isso o shuffle conta como uso do gerador, a cada chamada avanca o estado do rng, entao 
        gerar varios individuos em sequencia com o msm rng da resultados diferentes entre eles, mas reproduziveis entre execuções
        logo -> mesma seed inicial == mesma sequencia de individuos*/

        return individuo;
    }
}