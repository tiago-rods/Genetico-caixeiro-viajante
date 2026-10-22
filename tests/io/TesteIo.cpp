#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Verificacao.hpp"
#include "tsp/io/ArgParser.hpp"
#include "tsp/io/CsvWriter.hpp"

using namespace tsp;
using tsp::testes::verificar;

namespace {
    // ArgParser recebe char* (não const) como o argv de verdade
    io::ArgParser parsear(std::vector<std::string> argumentos) {
        std::vector<char*> argv;
        for (std::string& a : argumentos) argv.push_back(a.data());
        return io::ArgParser(static_cast<int>(argv.size()), argv.data());
    }

    template <typename Funcao>
    bool lanca(Funcao funcao) {
        try {
            funcao();
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    }
}

int main() {
    const io::ArgParser args = parsear({"tsp", "--n", "30", "--cenario", "circulo", "--mutacao", "0.25",
                                        "--ajuda", "--lista", "0,0.5,1"});
    verificar(args.inteiro("n", 0) == 30, "le inteiro");
    verificar(args.texto("cenario", "") == "circulo", "le texto");
    verificar(args.real("mutacao", 0.0) == 0.25, "le real");
    verificar(args.tem("ajuda") && args.texto("ajuda", "x").empty(), "flag sem valor");
    verificar(args.listaReais("lista", {}) == std::vector<double>{0.0, 0.5, 1.0}, "le lista de reais");
    verificar(args.inteiro("populacao", 123) == 123, "chave ausente devolve o padrao");
    verificar(args.chavesNaoUsadas().empty(), "nenhuma chave sobrando depois de ler todas");

    const io::ArgParser comErro = parsear({"tsp", "--n", "12abc", "--mutacaoo", "0.1"});
    verificar(lanca([&] { comErro.inteiro("n", 0); }), "inteiro mal formatado lanca invalid_argument");
    verificar(comErro.chavesNaoUsadas() == std::vector<std::string>{"--mutacaoo"},
              "chave digitada errado aparece como nao usada");
    verificar(lanca([] { parsear({"tsp", "solto"}); }), "argumento sem -- lanca invalid_argument");

    verificar(io::rotaComoTexto({3, 0, 2, 1}) == "3 0 2 1", "rota como texto separada por espaco");

    // CSV: cabeçalho + linhas, e linha com número errado de colunas é erro de programação
    const auto caminho = std::filesystem::temp_directory_path() / "tsp_teste_io.csv";
    {
        io::CsvWriter csv(caminho, {"a", "b", "c"});
        csv.linha(1, 2.5, "x");
        bool lancou = false;
        try {
            csv.linha(1, 2);
        } catch (const std::logic_error&) {
            lancou = true;
        }
        verificar(lancou, "linha com colunas a menos lanca logic_error");
    }
    std::ifstream arquivo(caminho);
    std::string cabecalho, primeira;
    std::getline(arquivo, cabecalho);
    std::getline(arquivo, primeira);
    verificar(cabecalho == "a,b,c" && primeira == "1,2.5,x", "CSV com cabecalho e valores separados por virgula");
    arquivo.close();
    std::filesystem::remove(caminho);

    return tsp::testes::resultadoFinal();
}
