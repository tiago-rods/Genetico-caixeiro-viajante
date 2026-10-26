# Divisão de trabalho (4 integrantes, 1 mês)

AG para o caixeiro viajante em 2D: núcleo em C++, gráficos em Python.
Início em 05/10/2026, entrega em 28/10/2026.

## Divisão

| Pessoa | Bloco | Itens que defende |
|---|---|---|
| Pessoa A | Problema, aptidão e infraestrutura | Aptidão (1) |
| Pessoa B | Representação e operadores | Gene e cruzamento (2) |
| Pessoa C | Laço evolutivo e experimentos | População e parada (1), mutação (1) |
| Pessoa D | Visualização, execução e relatório | Aleatório (2,5), circular (2,5), bônus (até 1) |

## Item da nota → evidência no relatório

| Item | Evidência | Dono |
|---|---|---|
| População e parada | Varredura de população; curvas mostrando onde a melhora para | Pessoa C |
| Mutação | Mapa de calor população × mutação; curvas com mutação baixa e alta | Pessoa C |
| Gene e cruzamento | Explicação do ERX; comparação ERX × OX | Pessoa B |
| Aptidão | Explicação de `1/distância` | Pessoa A |
| Aleatório | Curvas por época; rotas por época (grade e GIF); comparação com força bruta (N ≤ 10) e vizinho mais próximo; média ± desvio em ~10 seeds | Pessoa D |
| Circular | O mesmo, mais o gap (%) para o ótimo | Pessoa D |
| Bônus | Círculo com N grande: tempo até gap ≤ 5%, ≤ 1% e até a última melhora; rotas intermediárias | Pessoa D |

## Tarefas

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

### Pessoa B — Representação e operadores
- Feito: permutação de índices; torneio de tamanho `k`; ERX (prioriza arestas comuns aos dois pais, depois o menor grau); mutação por inversão (equivale a um 2-opt aleatório).
- Falta:
  - OX como segundo `ICruzamento`, para comparar com o ERX.
  - README e decisão 005 com a prioridade às arestas comuns (commit de 08/10).
  - Remover o comentário obsoleto em `ERX.cpp` (linha 36).

### Pessoa C — Laço evolutivo e experimentos
- `Parametros`: população, taxas de cruzamento e mutação, torneio, elite, máximo de épocas, estagnação, meta, seed, cruzamento.
- `AlgoritmoGenetico`: elitismo, seleção → cruzamento → mutação, estatísticas por época. Nova população em vetor separado.
- Parada: o primeiro entre máximo de épocas, estagnação e meta (gap no círculo).
- Mutação = probabilidade, por filho, de aplicar uma inversão.
- `main.cpp`: roda um cenário e imprime cada época.
- `experimentos.cpp`: população × mutação × cruzamento, ~10 seeds.
- Testes (CTest): o melhor nunca piora; círculo de 8 e força bruta atingem o ótimo; filhos e mutantes são permutações válidas.
- Opcional: diversidade da população por época.

### Pessoa D — Visualização, execução e relatório
- Feito (adaptado ao formato real do C++ em 26/10):
  - `viz/plotar.py`: curva melhor/média/pior e diversidade por época, grade de rotas,
    GIF, gap e marcos (tempo até gap ≤ 5%, ≤ 1% e última melhora).
  - `viz/referencias.py`: força bruta (N ≤ 10), vizinho mais próximo e referência
    2-opt (reaproveita `scripts/referencia_2opt.py`).
  - `viz/plotar_experimentos.py`: mapas de calor população × mutação, população por
    seed e critério de parada, a partir de `docs/experimentos/`.
  - `viz/tabela_seeds.py` (média ± desvio em 10 seeds) e `viz/comparar.py` (mutação
    baixa × alta).
  - `scripts/rodar_tudo.ps1`: compila, testa, roda os cenários e gera as figuras.
  - Cenários: uniforme e círculo com N = 10 e N = 50; bônus com círculo de N = 100,
    200 e 500.
  - `relatorio.md`: junta textos e figuras; a tabela de evidências é a checklist.
- Rodar os scripts com `uv run` (dependências no topo de cada um, PEP 723).
- Falta: figura ERX × OX, que depende do OX (Pessoa B).

## Arquivos de saída

Formato definido pelo C++ (`src/io/CsvWriter.cpp`; tabela "Arquivos" do README). Uma
pasta por execução do `tsp` (`--saida`, ex.: `saida/circulo_n50_s1/`).

| Arquivo | Formato |
|---|---|
| `pontos.csv` | `indice,x,y` |
| `historico.csv` | `geracao,melhor,media,pior,desvio,diversidade,tempo_ms` (geração 0 = população inicial; tempo acumulado) |
| `rotas.csv` | `geracao,distancia,tempo_ms,rota`, só quando a melhor rota melhora (a rota de uma época é a última linha com `geracao` ≤ época) |
| `resumo.txt` | `chave: valor`: parâmetros, `melhor_distancia`, `otimo` e `gap_percentual` (círculo), `geracoes`, `motivo_parada`, `geracao_ultima_melhora`, tempos |
| `varredura.csv`, `estagnacao.csv` | saídas do `experimentos` (em `docs/experimentos/`) |

Escala: uniforme em `[0, 100]²` (`--seed-pontos`, padrão 42, fixa os pontos);
círculo de raio 50 centrado em `(50, 50)`. Opções do `tsp`: `tsp --ajuda`.

Para testar os gráficos sem compilar: `scripts\rodar_tudo.ps1 -Exemplo` (gera dados
falsos no mesmo formato em `saida_exemplo/`).

## Dependências

- Pessoa A → todos: `.hpp` e formato dos arquivos na semana 1.
- Pessoa B → Pessoa C: operadores prontos; OX quando sair.
- Pessoas A e C → Pessoa D: arquivos reais a partir da semana 3.

## Cronograma (05/10 a 28/10)

| Semana | Datas | O que acontece |
|---|---|---|
| 1 | 05–11/10 | Formato dos arquivos, esqueleto que compila, operadores da Pessoa B (feito) |
| 2 | 12–18/10 | Pessoas A, C e D em paralelo; Pessoa B faz o OX |
| 3 | 19–23/10 | Integração, varreduras, valores finais, bônus; textos começam |
| 4 | 24–26/10 | Relatório; Pessoa D consolida |
| — | 27/10 | Revisão cruzada e ensaio |
| — | 28/10 | Entrega |

## Extras (fora da nota)

EAX-single, TSPLIB e 2-opt, só se sobrar tempo. O 2-opt resolveria o círculo
sozinho e esconderia o desempenho do AG.

## Riscos

- Atraso no formato dos arquivos trava todo mundo.
- Bônus pode não chegar ao ótimo: por isso mede-se o gap.
- `rotas.csv` ficaria grande com N alto: por isso o C++ grava só as melhorias.
- O `python` do PATH é o atalho da Microsoft Store: usar `uv run`.
- A mesma seed só reproduz com o mesmo compilador (g++ do MSYS2).
