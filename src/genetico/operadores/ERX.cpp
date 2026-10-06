#include "tsp/genetico/operadores/ERX.hpp"

#include <algorithm>
#include <random>
#include <set>
#include <vector>


namespace tsp::genetico {
        
    using TabelaVizinhos = std::vector<std::set<int>>;

    namespace {
        /*
        Processa Um pai e vai inserindo em uma tabela de adjacencias
        Para cada cidade na rota, o vizinho anterior e o proximo (circular, ou seja,
        a ultima cidade da rota é vizinha da primeira) são marcadas como vizinhas na tabela
        */

        void adicionarArestasPai(TabelaVizinhos& vizinhos, const Individuo& pai){
            std::size_t numCidades = pai.size();

            for (std::size_t i = 0; i < numCidades; ++i){
                int cidade = pai[i];
                int anterior = pai[(i + numCidades - 1) % numCidades]; // vizinho anterior, circular
                int proximo = pai[(i + 1) % numCidades]; // vizinho proximo, circular

                vizinhos[cidade].insert(anterior);
                vizinhos[cidade].insert(proximo);
            }
        }

        /*
        Monta a tabela de vizingos combinando as arestas dos dois pais
        Se ambos tiverem a mesma aresta, o set guarda somente uma vez (nao seria necessario capturar qual aresta veio de ambos nesse caso?)
        porém a lógica do ERX apenas se importa se a cidade está ou não no conjunto
        */

        TabelaVizinhos construirTabelaVizinhos(const Individuo& pai1, const Individuo& pai2){
            TabelaVizinhos vizinhos(pai1.size());

            adicionarArestasPai(vizinhos, pai1);
            adicionarArestasPai(vizinhos, pai2);

            return vizinhos;
        }
    }

    Individuo ERX::cruzar(const Individuo& pai1, const Individuo& pai2, std::mt19937& rng) const {
        std::size_t numCidades = pai1.size();
        TabelaVizinhos vizinhos = construirTabelaVizinhos(pai1, pai2);

        Individuo filho;
        filho.reserve(numCidades);

        std::vector<bool> visitado(numCidades, false);

        // convenção: começa pela primeira cidade do pai1
        int cidadeAtual = pai1[0];
        filho.push_back(cidadeAtual);
        visitado[cidadeAtual] = true;

        while (filho.size() < numCidades){
            // Copia os vizinhos da cidade atual antes de remover as referências dela na tabela, pois ainda vamos precisar dessa info
            std::set<int> vizinhosAtuais = vizinhos[cidadeAtual];

            // A relação é simétrica por construção, se A é vizinho de B, B é de A,
            // Então, remover a cidade atual de cada um dos seus vizinhos, já fecha a aresta dos dois lados
            // não precisa percorrer a tabela inteira para limpar isso

            for (int vizinho : vizinhosAtuais){
                vizinhos[vizinho].erase(cidadeAtual);
            }

            // entre os vizinhos da cidade atual, filtra só quem ainda não foi visitado

            std::vector<int> candidatos;
            for (int vizinho : vizinhosAtuais){
                if (!visitado[vizinho]) candidatos.push_back(vizinho);
            }

            int proximaCidade;

            if (candidatos.empty()){
                // nenhym vizinho disponível, logo esta preso nesse estado
                // sorteia qualquer cidade que ainda falta visitar
                std::vector<int> vizinhosNaoVisitados;
                for (std::size_t c = 0; c < numCidades; ++c){
                    if (!visitado[c]) vizinhosNaoVisitados.push_back(static_cast<int>(c));
                }

                std::uniform_int_distribution<std::size_t> distribuicao(0, vizinhosNaoVisitados.size() - 1);
                proximaCidade = vizinhosNaoVisitados[distribuicao(rng)];
            } else {
                // heurística do ERX: entre os candidatos, prioriza quem tem menos vizinhos restantes
                // reduz chance de ficar preso mais pra frente
                // se empatar, sorteia entre os empatados
                std::size_t menorGrau = numCidades;

                for (int candidato : candidatos){
                    menorGrau = std::min(menorGrau, vizinhos[candidato].size());
                }
                
                std::vector<int> melhoresCandidatos;
                for (int candidato : candidatos){
                    if (vizinhos[candidato].size() == menorGrau){
                        melhoresCandidatos.push_back(candidato);
                    }
                }

                std::uniform_int_distribution<std::size_t> distribuicao(0, melhoresCandidatos.size() - 1);
                proximaCidade = melhoresCandidatos[distribuicao(rng)];
        }

        filho.push_back(proximaCidade);
        visitado[proximaCidade] = true;
        cidadeAtual = proximaCidade;
    }
    return filho;
    }
}
