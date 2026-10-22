#include "tsp/io/ArgParser.hpp"

#include <sstream>
#include <stdexcept>

namespace tsp::io {

    namespace {
        bool ehChave(const std::string& argumento) {
            return argumento.size() > 2 && argumento.compare(0, 2, "--") == 0;
        }

        std::vector<std::string> separarPorVirgula(const std::string& texto) {
            std::vector<std::string> partes;
            std::stringstream fluxo(texto);
            std::string parte;
            while (std::getline(fluxo, parte, ',')) {
                if (!parte.empty()) partes.push_back(parte);
            }
            return partes;
        }

        // stoi/stod aceitam "12abc" e param no 'a'; aqui o texto inteiro tem que ser número
        int converterInteiro(const std::string& chave, const std::string& texto) {
            try {
                std::size_t lidos = 0;
                int valor = std::stoi(texto, &lidos);
                if (lidos == texto.size()) return valor;
            } catch (const std::exception&) {
                // cai no throw abaixo, com a mensagem que diz qual chave
            }
            throw std::invalid_argument("--" + chave + " espera um inteiro, recebeu '" + texto + "'");
        }

        double converterReal(const std::string& chave, const std::string& texto) {
            try {
                std::size_t lidos = 0;
                double valor = std::stod(texto, &lidos);
                if (lidos == texto.size()) return valor;
            } catch (const std::exception&) {
            }
            throw std::invalid_argument("--" + chave + " espera um numero, recebeu '" + texto + "'");
        }
    }

    ArgParser::ArgParser(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string argumento = argv[i];
            if (!ehChave(argumento)) {
                throw std::invalid_argument("argumento inesperado '" + argumento + "' (use --chave valor)");
            }

            std::string chave = argumento.substr(2);
            // sem valor depois (fim da lista ou outra --chave): é uma flag
            if (i + 1 < argc && !ehChave(argv[i + 1])) {
                valores_[chave] = argv[++i];
            } else {
                valores_[chave] = "";
            }
        }
    }

    bool ArgParser::tem(const std::string& chave) const {
        consultadas_.insert(chave);
        return valores_.count(chave) > 0;
    }

    std::string ArgParser::texto(const std::string& chave, const std::string& padrao) const {
        if (!tem(chave)) return padrao;
        return valores_.at(chave);
    }

    int ArgParser::inteiro(const std::string& chave, int padrao) const {
        if (!tem(chave)) return padrao;
        return converterInteiro(chave, valores_.at(chave));
    }

    double ArgParser::real(const std::string& chave, double padrao) const {
        if (!tem(chave)) return padrao;
        return converterReal(chave, valores_.at(chave));
    }

    std::vector<int> ArgParser::listaInteiros(const std::string& chave, const std::vector<int>& padrao) const {
        if (!tem(chave)) return padrao;
        std::vector<int> valores;
        for (const std::string& parte : separarPorVirgula(valores_.at(chave))) {
            valores.push_back(converterInteiro(chave, parte));
        }
        return valores;
    }

    std::vector<double> ArgParser::listaReais(const std::string& chave, const std::vector<double>& padrao) const {
        if (!tem(chave)) return padrao;
        std::vector<double> valores;
        for (const std::string& parte : separarPorVirgula(valores_.at(chave))) {
            valores.push_back(converterReal(chave, parte));
        }
        return valores;
    }

    std::vector<std::string> ArgParser::listaTextos(const std::string& chave, const std::vector<std::string>& padrao) const {
        if (!tem(chave)) return padrao;
        return separarPorVirgula(valores_.at(chave));
    }

    std::vector<std::string> ArgParser::chavesNaoUsadas() const {
        std::vector<std::string> naoUsadas;
        for (const auto& [chave, valor] : valores_) {
            if (consultadas_.count(chave) == 0) naoUsadas.push_back("--" + chave);
        }
        return naoUsadas;
    }
}
