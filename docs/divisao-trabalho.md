# Divisão de trabalho (4 integrantes, 2 semanas)

Projeto 2: algoritmo genético para o caixeiro viajante em 2D (núcleo em C++,
gráficos em Python). Datas assumem início em 05/10/2026 e entrega em
19/10/2026 — ajustar se o prazo real for outro.

## Critério da divisão

Cada pessoa é dona de um bloco de código **e** da explicação pontuada daquele
bloco, para que a nota fique distribuída e todos tenham o que defender.

| Pessoa | Bloco | Itens da nota que defende |
|---|---|---|
| A | Problema, aptidão e infraestrutura | Função de aptidão (1 pt) |
| B | Representação e operadores genéticos | Representação do gene e cruzamento (2 pts) |
| C | Laço evolutivo e experimentos | População e critério de parada (1 pt), taxa de mutação (1 pt) |
| D | Visualização, execução dos cenários e relatório | Desempenho por época: aleatório (2,5) e circular (2,5), bônus (até 1) |

## Divisão sugerida

### Pessoa A — Problema, aptidão e infraestrutura
- `CMakeLists.txt` (C++17, Ninja, link `-static`), `.gitignore`, esqueleto de
  pastas, `README.md` com como compilar e rodar.
- `tsp/`: `Ponto`, geração dos cenários (`gerarUniforme`, `gerarCirculo`),
  ótimo do círculo `2·N·R·sin(π/N)`, matriz de distâncias, `distanciaRota`
  (ciclo fechado).
- Função de aptidão: `1 / distância total`.
- `io/`: leitura dos argumentos da linha de comando e gravação de
  `pontos.csv`, `historico.csv`, `rotas.csv` e `resumo.txt`.
- Testes: quadrado unitário mede 4; ótimo do círculo confere com a soma das
  arestas.
- **Entrega primeiro (dias 1–2):** os `.hpp` com as assinaturas e o formato
  dos CSVs, porque B, C e D dependem disso.

### Pessoa B — Representação e operadores genéticos
- Cromossomo = permutação dos índices das cidades (`std::vector<int>`);
  geração de indivíduo aleatório.
- `Selecao`: torneio de tamanho `k`.
- `Cruzamento`: OX (Order Crossover).
- `Mutacao`: inversão de trecho.
- Testes: todo filho e todo mutante continua sendo permutação válida; mesma
  seed, mesmo resultado.
- Bloco mais isolado: só usa `std::vector<int>` e `std::mt19937`, então não
  espera ninguém.

### Pessoa C — Laço evolutivo e experimentos
- `Parametros` (população, taxas, torneio, elite, gerações, estagnação, seed).
- `AlgoritmoGenetico`: população inicial, avaliação, elitismo, seleção →
  cruzamento → mutação, registro das estatísticas por época, parada por teto
  de gerações ou por estagnação.
- `main.cpp`: liga cenário, AG e gravação; imprime o acompanhamento por época
  no terminal.
- `experimentos.cpp`: varredura de população × taxa de mutação com várias
  seeds, gravando um CSV.
- Medição de tempo (total e até a última melhora), usada no bônus.
- Testes: com elitismo o melhor nunca piora; círculo de 8 pontos atinge o
  ótimo.
- Pode começar com operadores provisórios (ex.: troca simples) e trocar pelos
  de B na integração.

### Pessoa D — Visualização, execução e relatório
- `viz/plotar.py`: curva de convergência (melhor e média por época), grade de
  snapshots da rota, GIF animado.
- `viz/plotar_experimentos.py`: gráficos da varredura de população e de
  mutação.
- `rodar_tudo.ps1`: compila, roda os três cenários e gera as figuras.
- Executa e documenta os cenários: uniforme, círculo (com gap para o ótimo) e
  bônus (círculo com muitos pontos, tempos e épocas intermediárias).
- `docs/relatorio.md`: junta as explicações escritas por A, B e C com as
  figuras e os números medidos.
- Desenvolve os gráficos desde o dia 1 com CSVs de exemplo escritos à mão no
  formato combinado.

## Dependências entre blocos

- **A → todos**: assinaturas e formato dos CSVs saem nos dias 1–2; o resto de
  A segue em paralelo.
- **B → C**: C integra os operadores de B no dia 7; antes disso usa
  provisórios.
- **A + C → D**: D só precisa de CSVs reais a partir do dia 7.

## Cronograma sugerido (05/10 a 19/10)

| Dias | Datas | O que acontece |
|---|---|---|
| 1–2 | 05–06/10 | Reunião de contrato: assinaturas, formato dos CSVs, parâmetros. A sobe o esqueleto que compila. |
| 3–6 | 07–10/10 | Os quatro blocos em paralelo, cada um com seus testes. |
| 7–8 | 11–12/10 | Integração: primeira execução ponta a ponta nos dois cenários, com figuras. |
| 9–10 | 13–14/10 | Varredura de população e mutação, escolha dos valores finais, execução do bônus. |
| 11–12 | 15–16/10 | Relatório: cada um escreve a explicação do seu bloco; D consolida. |
| 13 | 17/10 | Revisão cruzada e ensaio da arguição: todos conseguem explicar todos os itens. |
| 14 | 18/10 | Folga para imprevistos; entrega em 19/10. |

## Riscos

- **Contrato atrasar** trava os outros três: é a única tarefa com prazo rígido
  nos dias 1–2.
- **Bônus com muitos pontos pode não convergir** ao ótimo exato; vale reportar
  o gap e o tempo mesmo assim, e começar os testes no dia 9, não na véspera.
- **Python não está instalado no PATH** nesta máquina; rodar os scripts com
  `uv run` (que baixa Python e dependências) ou cada um instala por conta
  própria.
