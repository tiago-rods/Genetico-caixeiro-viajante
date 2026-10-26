#include "tsp/io/ArgParser.hpp"

#include <stdexcept> // invalid_argument, out_of_range

namespace tsp::io {
    namespace {
        bool ehChave(const std::string& token) {
            return token.size() >= 2 && token.compare(0, 2, "--") == 0;
        }

        /* Converte o texto inteiro ou lança erro. std::stoi("12abc") devolveria 12 sem reclamar,
        então confere se a conversão consumiu todos os caracteres */
        template <typename Conversor>
        auto converter(const std::string& chave, const std::string& texto, Conversor conversor, const char* tipo) {
            try {
                std::size_t consumidos = 0;
                auto valor = conversor(texto, &consumidos);
                if (consumidos != texto.size()) throw std::invalid_argument("sobrou texto");
                return valor;
            } catch (const std::exception&) { // invalid_argument ou out_of_range
                throw std::invalid_argument("--" + chave + " espera um " + tipo + ", recebeu '" + texto + "'");
            }
        }
    }

    ArgParser::ArgParser(int argc, char** argv) {
        // argv[0] é o nome do executável, por isso começa do 1
        for (int i = 1; i < argc; ++i) {
            std::string token = argv[i];
            if (!ehChave(token)) {
                throw std::invalid_argument("argumento inesperado '" + token + "', use --chave valor");
            }

            std::string chave = token.substr(2);
            if (chave.empty()) throw std::invalid_argument("chave vazia ('--')");

            // o valor é o próximo token; se não houver ou se já for outra chave, faltou o valor.
            // número negativo (-5) começa com um traço só, então não é confundido com chave
            if (i + 1 >= argc || ehChave(argv[i + 1])) {
                throw std::invalid_argument("--" + chave + " sem valor");
            }

            // insert devolve false se a chave já existia: chave repetida é quase sempre engano
            if (!valores_.insert({chave, argv[i + 1]}).second) {
                throw std::invalid_argument("--" + chave + " passada mais de uma vez");
            }
            ++i; // pula o valor que acabou de ser consumido
        }
    }

    bool ArgParser::tem(const std::string& chave) const {
        usadas_.insert(chave);
        return valores_.count(chave) > 0;
    }

    int ArgParser::obterInt(const std::string& chave, int padrao) const {
        if (!tem(chave)) return padrao;
        return converter(chave, valores_.at(chave),
                         [](const std::string& s, std::size_t* pos) { return std::stoi(s, pos); }, "inteiro");
    }

    double ArgParser::obterDouble(const std::string& chave, double padrao) const {
        if (!tem(chave)) return padrao;
        // stod usa o locale "C" (o programa nunca troca o locale): decimal com ponto, 0.05
        return converter(chave, valores_.at(chave),
                         [](const std::string& s, std::size_t* pos) { return std::stod(s, pos); }, "numero");
    }

    std::string ArgParser::obterString(const std::string& chave, const std::string& padrao) const {
        if (!tem(chave)) return padrao;
        return valores_.at(chave);
    }

    std::vector<std::string> ArgParser::chavesNaoUsadas() const {
        std::vector<std::string> naoUsadas;
        for (const auto& [chave, valor] : valores_) {
            if (usadas_.count(chave) == 0) naoUsadas.push_back(chave);
        }
        return naoUsadas;
    }
}
