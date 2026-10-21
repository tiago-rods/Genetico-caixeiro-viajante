# Divisão de trabalho (4 integrantes, 1 mês)

AG para o caixeiro viajante em 2D: núcleo em C++, gráficos em Python.
Início em 05/10/2026, entrega em 28/10/2026.

## Divisão

| Pessoa | Bloco | Itens que defende |
|---|---|---|
| A | Problema, aptidão e infraestrutura | Aptidão (1) |
| B | Representação e operadores | Gene e cruzamento (2) |
| C | Laço evolutivo e experimentos | População e parada (1), mutação (1) |
| D | Visualização, execução e relatório | Aleatório (2,5), circular (2,5), bônus (até 1) |

## Item da nota → evidência no relatório

| Item | Evidência | Dono |
|---|---|---|
| População e parada | Varredura de população; curvas mostrando onde a melhora para | C |
| Mutação | Mapa de calor população × mutação; curvas com mutação baixa e alta | C |
| Gene e cruzamento | Explicação do ERX; comparação ERX × OX | B |
| Aptidão | Explicação de `1/distância` | A |
| Aleatório | Curvas por época; rotas por época (grade e GIF); comparação com força bruta (N ≤ 10) e vizinho mais próximo; média ± desvio em ~10 seeds | D |
| Circular | O mesmo, mais o gap (%) para o ótimo | D |
| Bônus | Círculo com N grande: tempo até gap ≤ 5%, ≤ 1% e até a última melhora; rotas intermediárias | D |

## Tarefas

### A — Problema, aptidão e infraestrutura
- `CMakeLists.txt` (C++17, Ninja, `-static`), `.gitignore`, README com como rodar.
- `Ponto`, `gerarUniforme`, `gerarCirculo`, matriz de distâncias, `distanciaRota` (ciclo fechado). N arbitrário (≥ 8) pela linha de comando.
- Círculo com pontos igualmente espaçados (ótimo = `2·N·R·sin(π/N)`) e índices embaralhados.
- Referências para o uniforme: força bruta (N ≤ 10) e vizinho mais próximo.
- Aptidão `1/distância`. Como o torneio só compara, equivale a usar `−d`.
- `io/`: argumentos e arquivos de saída (ver abaixo).
- Testes: quadrado unitário mede 4; ótimo do círculo confere.
- **Semana 1:** `.hpp` e formato dos arquivos, porque todos dependem disso.

### B — Representação e operadores
- Feito: permutação de índices; torneio de tamanho `k`; ERX (prioriza arestas comuns aos dois pais, depois o menor grau); mutação por inversão (equivale a um 2-opt aleatório).
- Falta:
  - OX como segundo `ICruzamento`, para comparar com o ERX.
  - README e decisão 005 com a prioridade às arestas comuns (commit de 08/10).
  - Remover o comentário obsoleto em `ERX.cpp` (linha 36).

### C — Laço evolutivo e experimentos
- `Parametros`: população, taxas de cruzamento e mutação, torneio, elite, máximo de épocas, estagnação, meta, seed, cruzamento.
- `AlgoritmoGenetico`: elitismo, seleção → cruzamento → mutação, estatísticas por época. Nova população em vetor separado.
- Parada: o primeiro entre máximo de épocas, estagnação e meta (gap no círculo).
- Mutação = probabilidade, por filho, de aplicar uma inversão.
- `main.cpp`: roda um cenário e imprime cada época.
- `experimentos.cpp`: população × mutação × cruzamento, ~10 seeds.
- Testes (CTest): o melhor nunca piora; círculo de 8 e força bruta atingem o ótimo; filhos e mutantes são permutações válidas.
- Opcional: diversidade da população por época.

### D — Visualização, execução e relatório
- `plotar.py`: curva melhor/média/pior por época, grade de rotas, GIF.
- `plotar_experimentos.py`: mapa de calor, ERX × OX, boxplot das seeds.
- Rodar com `uv run` (dependências no topo de cada script, PEP 723).
- `rodar_tudo.ps1`: compila, roda e gera as figuras.
- Cenários: uniforme e círculo com N ≤ 10 e N ≈ 30–50; bônus com círculo de N = 100 a 1000.
- `relatorio.md`: junta textos e figuras; usar a tabela de evidências como checklist.
- Começa com CSVs de exemplo no formato abaixo.

## Arquivos de saída

Uma pasta por execução (ex.: `saida/circulo_n50_s1/`).

| Arquivo | Formato |
|---|---|
| `pontos.csv` | `indice,x,y` |
| `historico.csv` | `epoca,melhor,media,pior,tempo_s` (época 0 = população inicial; tempo acumulado) |
| `rotas.csv` | `epoca,distancia,rota` (índices separados por espaço; toda época, ou a cada `k` com `--intervalo-rotas`) |
| `resumo.txt` | `chave=valor`: parâmetros, melhor distância, ótimo ou referência, gap, motivo da parada, tempos |
| `experimentos.csv` | `cenario,n,populacao,taxa_mutacao,cruzamento,seed,melhor,epocas,tempo_s` |

Argumentos: `--cenario`, `--n`, `--seed`, `--populacao`, `--taxa-cruzamento`,
`--taxa-mutacao`, `--torneio`, `--elite`, `--max-epocas`, `--estagnacao`,
`--cruzamento erx|ox`, `--intervalo-rotas`, `--saida`.

## Dependências

- A → todos: `.hpp` e formato dos arquivos na semana 1.
- B → C: operadores prontos; OX quando sair.
- A + C → D: arquivos reais a partir da semana 3.

## Cronograma (05/10 a 28/10)

| Semana | Datas | O que acontece |
|---|---|---|
| 1 | 05–11/10 | Formato dos arquivos, esqueleto que compila, operadores do B (feito) |
| 2 | 12–18/10 | A, C e D em paralelo; B faz o OX |
| 3 | 19–23/10 | Integração, varreduras, valores finais, bônus; textos começam |
| 4 | 24–26/10 | Relatório; D consolida |
| — | 27/10 | Revisão cruzada e ensaio |
| — | 28/10 | Entrega |

## Extras (fora da nota)

EAX-single, TSPLIB e 2-opt, só se sobrar tempo. O 2-opt resolveria o círculo
sozinho e esconderia o desempenho do AG.

## Riscos

- Atraso no formato dos arquivos trava todo mundo.
- Bônus pode não chegar ao ótimo: por isso mede-se o gap.
- `rotas.csv` fica grande com N alto: usar `--intervalo-rotas`.
- O `python` do PATH é o atalho da Microsoft Store: usar `uv run`.
- A mesma seed só reproduz com o mesmo compilador (g++ do MSYS2).
