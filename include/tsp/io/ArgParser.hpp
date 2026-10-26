#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace tsp::io {
    /* Lê a linha de comando no formato "--chave valor" ou "--flag" sozinha.
    Cada getter recebe o valor padrão, então quem chama declara os defaults ali mesmo.
    Valor mal formatado (ex.: --n abc) lança std::invalid_argument dizendo qual chave */
    class ArgParser {
    public:
        ArgParser(int argc, char* argv[]);

        bool tem(const std::string& chave) const;

        std::string texto(const std::string& chave, const std::string& padrao) const;
        int inteiro(const std::string& chave, int padrao) const;
        double real(const std::string& chave, double padrao) const;

        // listas separadas por vírgula, ex.: --mutacoes 0,0.1,0.3
        std::vector<int> listaInteiros(const std::string& chave, const std::vector<int>& padrao) const;
        std::vector<double> listaReais(const std::string& chave, const std::vector<double>& padrao) const;
        std::vector<std::string> listaTextos(const std::string& chave, const std::vector<std::string>& padrao) const;

        /* Chaves passadas que nenhum getter consultou: quase sempre erro de digitação
        (--mutaçao em vez de --mutacao), que sem essa checagem passaria calado com o valor padrão */
        std::vector<std::string> chavesNaoUsadas() const;

    private:
        std::map<std::string, std::string> valores_;
        mutable std::set<std::string> consultadas_; // mutable: consultar não muda os argumentos, só anota o uso
    };
}
