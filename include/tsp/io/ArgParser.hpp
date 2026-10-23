#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace tsp::io {
    /* Lê argumentos da linha de comando no formato --chave valor (ex.: --n 50 --cenario circulo).
    É genérico de propósito: não conhece os Parametros do bloco C, só guarda texto e converte
    sob demanda. Assim C pode criar ou renomear parâmetros sem precisar mexer no bloco A */
    class ArgParser {
    public:
        // lança std::invalid_argument se os argumentos estiverem malformados
        // (token sem "--", chave sem valor, chave repetida)
        ArgParser(int argc, char** argv);

        bool tem(const std::string& chave) const;

        // devolvem o padrão se a chave não foi passada;
        // lançam std::invalid_argument se foi passada mas o valor não converte (ex.: --n abc)
        int obterInt(const std::string& chave, int padrao) const;
        double obterDouble(const std::string& chave, double padrao) const;
        std::string obterString(const std::string& chave, const std::string& padrao) const;

        /* Chaves passadas na linha de comando mas nunca consultadas pelo programa.
        Pega erro de digitação: com --geracos 500 o programa usaria o padrão de gerações
        sem avisar, assim o main pode listar essas chaves e abortar */
        std::vector<std::string> chavesNaoUsadas() const;

    private:
        std::map<std::string, std::string> valores_;
        // mutable: consultar uma chave (método const) só registra que ela foi usada,
        // não muda os valores lidos
        mutable std::set<std::string> usadas_;
    };
}
