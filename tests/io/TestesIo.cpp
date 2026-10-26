#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Assercoes.hpp"
#include "tsp/io/ArgParser.hpp"
#include "tsp/io/CsvWriter.hpp"

using namespace tsp::io;
using tsp::testes::quaseIgual;
using tsp::testes::verificar;
namespace fs = std::filesystem;

namespace {
    /* Monta um ArgParser a partir de uma lista de strings, simulando a linha de comando.
    O vetor de strings precisa viver enquanto o argv é usado, por isso fica dentro da struct */
    struct LinhaDeComando {
        std::vector<std::string> tokens;
        std::vector<char*> argv;

        explicit LinhaDeComando(std::vector<std::string> args) : tokens(std::move(args)) {
            tokens.insert(tokens.begin(), "tsp.exe"); // argv[0] é o nome do executável
            for (std::string& t : tokens) argv.push_back(t.data());
        }

        ArgParser criar() { return ArgParser(static_cast<int>(argv.size()), argv.data()); }
    };

    template <typename Excecao, typename Funcao>
    bool lanca(Funcao funcao) {
        try {
            funcao();
        } catch (const Excecao&) {
            return true;
        }
        return false;
    }

    std::vector<std::string> lerLinhas(const fs::path& arquivo) {
        std::ifstream entrada(arquivo);
        std::vector<std::string> linhas;
        for (std::string linha; std::getline(entrada, linha);) linhas.push_back(linha);
        return linhas;
    }

    void testarArgParserLeitura() {
        LinhaDeComando cmd({"--cenario", "circulo", "--n", "50", "--mutacao", "0.05", "--x", "-5"});
        ArgParser args = cmd.criar();

        verificar(args.obterString("cenario", "uniforme") == "circulo", "le string");
        verificar(args.obterInt("n", 8) == 50, "le inteiro");
        verificar(quaseIgual(args.obterDouble("mutacao", 0.1), 0.05), "le double com ponto decimal");
        verificar(args.obterInt("x", 0) == -5, "numero negativo e valor, nao chave");
        verificar(args.tem("n") && !args.tem("seed"), "tem() diferencia chave passada de ausente");
        verificar(args.obterInt("seed", 42) == 42, "chave ausente devolve o padrao");
        verificar(args.obterString("saida", "resultados") == "resultados", "string ausente devolve o padrao");
    }

    void testarArgParserErros() {
        verificar(lanca<std::invalid_argument>([] { LinhaDeComando({"--n"}).criar(); }),
                  "chave sem valor no fim lanca erro");
        verificar(lanca<std::invalid_argument>([] { LinhaDeComando({"--n", "--seed", "1"}).criar(); }),
                  "chave seguida de outra chave lanca erro");
        verificar(lanca<std::invalid_argument>([] { LinhaDeComando({"50"}).criar(); }),
                  "valor sem chave lanca erro");
        verificar(lanca<std::invalid_argument>([] { LinhaDeComando({"--", "1"}).criar(); }),
                  "chave vazia lanca erro");
        verificar(lanca<std::invalid_argument>([] { LinhaDeComando({"--n", "1", "--n", "2"}).criar(); }),
                  "chave repetida lanca erro");

        LinhaDeComando cmd({"--n", "abc", "--m", "12abc", "--k", "99999999999", "--t", "0,05"});
        ArgParser args = cmd.criar();
        verificar(lanca<std::invalid_argument>([&] { args.obterInt("n", 0); }), "inteiro invalido lanca erro");
        verificar(lanca<std::invalid_argument>([&] { args.obterInt("m", 0); }), "inteiro com lixo no fim lanca erro");
        verificar(lanca<std::invalid_argument>([&] { args.obterInt("k", 0); }), "inteiro fora do limite lanca erro");
        verificar(lanca<std::invalid_argument>([&] { args.obterDouble("t", 0.0); }),
                  "double com virgula decimal lanca erro (formato e com ponto)");
    }

    void testarChavesNaoUsadas() {
        LinhaDeComando cmd({"--n", "50", "--geracos", "500"}); // erro de digitação proposital
        ArgParser args = cmd.criar();
        args.obterInt("n", 8);
        args.obterInt("geracoes", 1000);

        std::vector<std::string> naoUsadas = args.chavesNaoUsadas();
        verificar(naoUsadas.size() == 1 && naoUsadas[0] == "geracos", "detecta chave digitada errado");
    }

    void testarPontos(const fs::path& pasta) {
        fs::path arquivo = pasta / "subpasta" / "pontos.csv"; // subpasta ainda não existe
        escreverPontos(arquivo, {{37.454012, 95.071431}, {0.0, 1.5}});

        std::vector<std::string> linhas = lerLinhas(arquivo);
        verificar(fs::exists(arquivo), "cria a pasta do arquivo se ela nao existir");
        verificar(linhas.size() == 3, "pontos.csv: cabecalho + 1 linha por ponto");
        verificar(linhas[0] == "indice,x,y", "pontos.csv: cabecalho");
        verificar(linhas[1] == "0,37.454012,95.071431", "pontos.csv: linha com ponto decimal e 6 casas");
        verificar(linhas[2] == "1,0.000000,1.500000", "pontos.csv: indice sequencial");
    }

    void testarHistorico(const fs::path& pasta) {
        fs::path arquivo = pasta / "historico.csv";
        escreverHistorico(arquivo, {{0, 812.4431, 1043.221, 1290.0012, 0.84}, {1, 0.00001, 2.0, 3.0, 1.5}});

        std::vector<std::string> linhas = lerLinhas(arquivo);
        verificar(linhas.size() == 3, "historico.csv: cabecalho + 1 linha por epoca");
        verificar(linhas[0] == "epoca,melhor,media,pior,tempo_ms", "historico.csv: cabecalho");
        verificar(linhas[1] == "0,812.443100,1043.221000,1290.001200,0.840000", "historico.csv: linha");
        verificar(linhas[2].rfind("1,0.000010,", 0) == 0, "historico.csv: numero pequeno sem notacao cientifica");
    }

    void testarRotas(const fs::path& pasta) {
        fs::path arquivo = pasta / "rotas.csv";
        escreverRotas(arquivo, {{0, 1043.221, {4, 0, 7, 2, 5, 1, 6, 3}}});

        std::vector<std::string> linhas = lerLinhas(arquivo);
        verificar(linhas.size() == 2, "rotas.csv: cabecalho + 1 linha por snapshot");
        verificar(linhas[0] == "epoca,distancia,rota", "rotas.csv: cabecalho");
        verificar(linhas[1] == "0,1043.221000,4 0 7 2 5 1 6 3", "rotas.csv: rota separada por espaco, sem repetir a 1a cidade");
    }

    void testarResumo(const fs::path& pasta) {
        Resumo circulo;
        circulo.cenario = "circulo";
        circulo.numCidades = 8;
        circulo.seed = 42;
        circulo.epocas = 153;
        circulo.melhorDistancia = 6.3;
        circulo.otimo = 6.0;
        circulo.tempoTotalMs = 12.4;
        circulo.epocaUltimaMelhora = 37;
        circulo.tempoAteUltimaMelhoraMs = 3.1;
        circulo.parametros = {{"populacao", "100"}, {"mutacao", "0.05"}};
        escreverResumo(pasta / "resumo_circulo.txt", circulo);

        std::vector<std::string> esperado = {
            "cenario=circulo", "num_cidades=8", "seed=42", "epocas=153", "melhor_distancia=6.300000",
            "otimo=6.000000", "gap_percentual=5.000000", "tempo_total_ms=12.400000",
            "epoca_ultima_melhora=37", "tempo_ate_ultima_melhora_ms=3.100000", "populacao=100", "mutacao=0.05"};
        verificar(lerLinhas(pasta / "resumo_circulo.txt") == esperado, "resumo.txt do circulo com otimo e gap de 5%");

        Resumo uniforme = circulo;
        uniforme.cenario = "uniforme";
        uniforme.otimo = std::nullopt;
        escreverResumo(pasta / "resumo_uniforme.txt", uniforme);

        bool temOtimoOuGap = false;
        for (const std::string& linha : lerLinhas(pasta / "resumo_uniforme.txt")) {
            if (linha.rfind("otimo=", 0) == 0 || linha.rfind("gap_percentual=", 0) == 0) temOtimoOuGap = true;
        }
        verificar(!temOtimoOuGap, "resumo.txt do uniforme nao tem otimo nem gap");
    }

    void testarErroDeEscrita(const fs::path& pasta) {
        // o caminho é uma pasta que já existe: não dá pra abrir como arquivo
        verificar(lanca<std::runtime_error>([&] { escreverPontos(pasta, {{0, 0}}); }),
                  "falha ao abrir o arquivo lanca runtime_error");
    }
}

int main() {
    testarArgParserLeitura();
    testarArgParserErros();
    testarChavesNaoUsadas();

    // pasta temporária do sistema: o teste não deixa lixo dentro do repositório
    fs::path pasta = fs::temp_directory_path() / "tsp_testes_io";
    fs::remove_all(pasta);
    fs::create_directories(pasta);

    testarPontos(pasta);
    testarHistorico(pasta);
    testarRotas(pasta);
    testarResumo(pasta);
    testarErroDeEscrita(pasta);

    fs::remove_all(pasta);
    return tsp::testes::resultado();
}
