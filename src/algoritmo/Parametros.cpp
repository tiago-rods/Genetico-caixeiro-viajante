#include "tsp/algoritmo/Parametros.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace tsp::algoritmo {

    namespace {
        void exigir(bool condicao, const std::string& mensagem) {
            if (!condicao) throw std::invalid_argument(mensagem);
        }

        bool ehProbabilidade(double taxa) {
            return taxa >= 0.0 && taxa <= 1.0;
        }
    }

    int estagnacaoPadrao(int numCidades) {
        return std::max(Parametros{}.maxEstagnacao, 3 * numCidades);
    }

    void validar(const Parametros& p) {
        exigir(p.tamanhoPopulacao >= 2, "populacao precisa de pelo menos 2 individuos");
        exigir(ehProbabilidade(p.taxaCruzamento), "taxa de cruzamento precisa estar em [0, 1]");
        exigir(ehProbabilidade(p.taxaMutacao), "taxa de mutacao precisa estar em [0, 1]");
        exigir(p.tamanhoTorneio >= 1, "torneio precisa de pelo menos 1 participante");
        // elite do tamanho da população = ninguém novo nasce e o AG fica parado
        exigir(p.tamanhoElite >= 0 && p.tamanhoElite < p.tamanhoPopulacao,
               "elite precisa estar em [0, populacao - 1]");
        exigir(p.maxGeracoes >= 1, "limite de geracoes precisa ser pelo menos 1");
        exigir(p.maxEstagnacao >= 1, "limite de estagnacao precisa ser pelo menos 1");
    }
}
