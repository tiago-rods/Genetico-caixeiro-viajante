#include <cmath>
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

    // ---- casos portados da branch pessoa-A ----

    // ArgParser: valores e chaves ausentes
    const io::ArgParser outros = parsear({"tsp", "--x", "-5"});
    verificar(outros.inteiro("x", 0) == -5, "numero negativo e valor, nao chave");
    verificar(!outros.tem("seed"), "tem() devolve false para chave ausente");
    verificar(outros.texto("saida", "resultados") == "resultados", "texto ausente devolve o padrao");

    // ArgParser: entradas que precisam virar erro, e não um valor silencioso
    verificar(lanca([] { parsear({"tsp", "--", "1"}); }), "'--' sozinho lanca invalid_argument");
    const io::ArgParser invalidos = parsear({"tsp", "--n", "abc", "--k", "99999999999", "--t", "0,05"});
    verificar(lanca([&] { invalidos.inteiro("n", 0); }), "inteiro nao numerico lanca invalid_argument");
    verificar(lanca([&] { invalidos.inteiro("k", 0); }), "inteiro fora do limite lanca invalid_argument");
    verificar(lanca([&] { invalidos.real("t", 0.0); }), "real com virgula decimal lanca invalid_argument (formato e com ponto)");

    const auto pasta = std::filesystem::temp_directory_path() / "tsp_teste_io_portado";
    std::filesystem::create_directories(pasta);
    auto lerLinhas = [](const std::filesystem::path& arquivo) {
        std::ifstream entrada(arquivo);
        std::vector<std::string> linhas;
        for (std::string linha; std::getline(entrada, linha);) linhas.push_back(linha);
        return linhas;
    };

    // pontos.csv: cabeçalho e ponto decimal (locale "C"), no formato do gravarPontos
    io::gravarPontos(pasta / "pontos.csv", {{37.454012, 95.071431}, {0.0, 1.5}});
    const std::vector<std::string> pontos = lerLinhas(pasta / "pontos.csv");
    verificar(pontos == std::vector<std::string>{"indice,x,y", "0,37.454012,95.071431", "1,0,1.5"},
              "pontos.csv com cabecalho, indice sequencial e ponto decimal");

    // resumo.txt: ótimo e gap só aparecem quando o cenário tem ótimo
    {
        algoritmo::Resultado resultado;
        resultado.melhorDistancia = 6.3;
        const core::Cenario circulo{"circulo", {}, 6.0};
        const core::Cenario uniforme{"uniforme", {}, std::nullopt};
        io::gravarResumo(pasta / "resumo_circulo.txt", circulo, 1, algoritmo::Parametros{}, resultado);
        io::gravarResumo(pasta / "resumo_uniforme.txt", uniforme, 1, algoritmo::Parametros{}, resultado);

        double gap = -1.0;
        for (const std::string& linha : lerLinhas(pasta / "resumo_circulo.txt")) {
            if (linha.rfind("gap_percentual: ", 0) == 0) gap = std::stod(linha.substr(16));
        }
        verificar(std::fabs(gap - 5.0) < 1e-9, "resumo do circulo tem gap de 5% para 6.3 contra otimo 6");

        bool temOtimoOuGap = false;
        for (const std::string& linha : lerLinhas(pasta / "resumo_uniforme.txt")) {
            temOtimoOuGap = temOtimoOuGap || linha.rfind("otimo:", 0) == 0 || linha.rfind("gap_percentual:", 0) == 0;
        }
        verificar(!temOtimoOuGap, "resumo do uniforme nao tem otimo nem gap");
    }

    // caminho que não dá pra abrir como arquivo (é uma pasta) vira erro, não arquivo vazio
    {
        bool lancou = false;
        try {
            io::gravarPontos(pasta, {{0, 0}});
        } catch (const std::runtime_error&) {
            lancou = true;
        }
        verificar(lancou, "falha ao abrir o arquivo lanca runtime_error");
    }
    std::filesystem::remove_all(pasta);

    return tsp::testes::resultadoFinal();
}
