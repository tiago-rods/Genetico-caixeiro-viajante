# Documentação de decisões

Registro curto de decisões de arquitetura e de projeto que não ficam
óbvias só lendo o código — o porquê, o que foi descartado e o trade-off
aceito. Serve de apoio pra defender essas escolhas na arguição.

Formato de cada entrada: **Contexto** (o problema), **Decisão** (o que
foi escolhido), **Alternativas consideradas** (e por que não), 
**Consequências** (o que isso custa ou evita), **Data**.

---

## 001 — Pastas por responsabilidade + interfaces para os operadores genéticos

- **Contexto**: divisão em 4 blocos (A/B/C/D, ver `divisao-trabalho.md`)
  com dependências entre eles (A → todos; B → C). Precisávamos de uma
  estrutura que deixasse essas dependências fracas e permitisse C
  começar com operadores provisórios enquanto B termina os dele.
- **Decisão**: `include/` e `src/` separados por responsabilidade
  (`core/`, `io/` — pessoa A; `genetico/` — pessoa B; `algoritmo/` —
  pessoa C), e os operadores genéticos expostos como interfaces
  (`ISelecao`, `ICruzamento`, `IMutacao`) das quais `AlgoritmoGenetico`
  depende — nunca de uma implementação concreta.
- **Alternativas consideradas**: uma pasta por pessoa (`src/pessoa-a/`...)
  — rejeitada, mistura responsabilidades por "quem" em vez de "o quê" e
  não guia ninguém se alguém trocar de bloco. Poucos arquivos grandes
  por bloco — rejeitada, viola responsabilidade única e dificulta os
  testes unitários isolados que o enunciado pede.
- **Consequências**: C pode injetar operadores provisórios e trocar
  pelos de B sem alterar `AlgoritmoGenetico` (troca só a implementação
  injetada). Também é o que tornou a decisão 002 (trocar o crossover)
  barata — é só adicionar outro `ICruzamento`, sem tocar no laço
  evolutivo.
- **Data**: 2026-10-05

## 002 — ERX em vez de OX como crossover principal

- **Contexto**: a maioria das equipes da turma provavelmente vai usar OX
  (Order Crossover), que é o mais didático. Queríamos algo diferente,
  mas viável no prazo de 2 semanas.
- **Decisão**: usar ERX (Edge Recombination) como cruzamento principal
  de B (`genetico/operadores/ERX.hpp/.cpp`), atrás da mesma interface
  `ICruzamento`. ERX monta, pra cada cidade, a lista de vizinhos a
  partir dos dois pais e percorre priorizando arestas comuns, com
  fallback aleatório em beco sem saída (lista vazia).
- **Alternativas consideradas**: OX — mais simples, mas é o que "todo
  mundo" ia usar. EAX completo — muito complexo pro prazo (decompor em
  AB-cycles e remontar subciclos num ciclo hamiltoniano válido). EAX de
  um único AB-cycle — ainda exige esse passo de remontagem, mais
  complexo que ERX sem ganho claro pro esforço que sobra.
- **Consequências**: ERX preserva arestas dos pais (o que importa pra
  distância da rota) em vez de posições, mas converge pra estrutura
  comum dos pais mais rápido que OX, perdendo diversidade da população
  mais rápido — por isso a taxa de mutação (bloco de C) provavelmente
  precisa ficar um pouco mais alta que com OX. Testes de validade de
  permutação continuam simples porque ERX constrói o filho direto, sem
  precisar de reparo.
- **Data**: 2026-10-05

## 003 — EAX-single, TSPLIB e métricas extras como bônus condicional

- **Contexto**: ao discutir a decisão 002, surgiu a ideia de ir além —
  usar EAX-single como segundo crossover, benchmarks TSPLIB com ótimo
  conhecido (berlin52, eil51, kroA100, ch150), métricas de edge
  inheritance e diversidade da população, visualização dos AB-cycles, e
  2-opt memético.
- **Decisão**: nenhum desses itens entra no escopo obrigatório. Ficam
  documentados em `divisao-trabalho.md` como bloco condicional, só
  iniciado depois que ERX + cenários aleatório/círculo estiverem
  rodando ponta a ponta, testados e com gráficos — não antes dos dias
  9–10 do cronograma.
- **Alternativas consideradas**: adotar EAX como crossover principal —
  rejeitada, risco alto pro prazo competindo com o que vale a maior
  parte da nota. Ignorar essas ideias completamente — rejeitada, porque
  TSPLIB e as métricas são baratas de adicionar (reaproveitam as
  abstrações já existentes) e fortalecem o relatório mesmo sem EAX.
- **Consequências**: tudo isso junto vale no máximo 1 ponto de bônus,
  contra 9 pontos do escopo obrigatório — se o prazo apertar, abandonar
  sem culpa. A decisão 001 (interfaces) já deixa o EAX plugável como
  segundo `ICruzamento` sem refatorar o laço evolutivo, se decidirem
  tentar.
- **Data**: 2026-10-05

## 004 — Implementação do bloco B: `Individuo`, `Torneio`, `ERX`, `InversaoMutacao`

- **Contexto**: bloco B (representação do cromossomo + os três operadores
  genéticos atrás de `ISelecao`/`ICruzamento`/`IMutacao`) implementado e
  revisado linha a linha. Algumas escolhas de implementação não ficam
  óbvias só lendo o código final, vale registrar.
- **Decisão**:
  - `Torneio` sorteia os `k` candidatos **com reposição** (o mesmo índice
    pode saír mais de uma vez) — simplifica a implementação; o custo de
    eventualmente repetir um candidato é desprezível pra `k` pequeno
    frente ao tamanho da população.
  - `InversaoMutacao` sorteia o índice `i` e um deslocamento em
    distribuições **separadas**, a segunda com range `[1, size-1]` (não
    `[0, size-1]`), e soma com módulo pra achar `j`. Isso garante
    `j != i` (evitando mutação nula) numa única sorteada, sem laço de
    rejeição e sem risco de `j` estourar o vetor.
  - `ERX` guarda a tabela de vizinhos como `std::vector<std::set<int>>`
    (no máximo 4 vizinhos por cidade — 2 de cada pai). O `set` resolve de
    graça tanto a deduplicação de arestas comuns aos dois pais quanto a
    remoção O(log n) durante a caminhada. A escolha do próximo passo
    prioriza o candidato com menor grau restante (heurística clássica do
    ERX), com desempate aleatório entre os de grau mínimo e fallback
    aleatório entre as cidades não visitadas quando não há candidato
    (beco sem saída).
- **Alternativas consideradas**: torneio sem reposição — mais "correto"
  estatisticamente, mas complexidade extra (controlar quais índices já
  saíram) sem ganho relevante pra `k` pequeno. Laço de rejeição pra
  `InversaoMutacao` (sortear `j` de novo enquanto `j == i`) — rejeitado a
  favor do deslocamento com range `[1, size-1]`, que resolve em tempo
  constante garantido, sem laço. `std::unordered_set` na tabela do ERX —
  rejeitado, o overhead de hashing não compensa pra conjuntos de tamanho
  ≤ 4.
- **Consequências**: os três operadores e a geração de indivíduo foram
  conferidos manualmente contra `ehPermutacaoValida`
  (`tests/genetico/VerificarPermutacao.hpp`) e por revisão de código
  linha a linha; bloco B pronto pra integração com C via as interfaces
  (`AlgoritmoGenetico` não depende de nenhuma implementação concreta).
- **Data**: 2026-10-05

## 005 — Implementação do bloco A: `core`, `io` e build

- **Contexto**: o bloco A (CMake, cenários, distâncias, aptidão, leitura de
  argumentos e gravação dos CSVs) ainda estava só no esqueleto quando o
  bloco C foi integrado, e sem ele nada compilava nem rodava. Foi
  implementado junto com o C, seguindo o contrato descrito em
  `divisao-trabalho.md`.
- **Decisão**:
  - `core` não depende de `genetico`: `distanciaRota` recebe
    `std::vector<int>` (que é o que `Individuo` é), então o problema não
    conhece o algoritmo.
  - `MatrizDistancias` pré-calcula as n² distâncias num `vector` contíguo
    (`linha * n + coluna`). Avaliar uma rota vira n leituras, sem raiz.
  - Uniforme em `[0, 100]²` e círculo de raio 50 centrado em `(50, 50)`:
    mesma caixa, gráficos comparáveis. Ótimo do círculo pela fórmula
    `2·n·r·sin(π/n)`, guardado em `Cenario::otimo` (`std::optional`,
    vazio no uniforme). Os pontos do círculo saem em ordem angular: como a
    população inicial é embaralhada e nenhum operador olha para o valor do
    índice, isso não dá vantagem ao AG.
  - `io::CsvWriter` é genérico (cabeçalho + `linha(...)` variádico) e as
    funções `gravarPontos/Historico/Rotas/Resumo` fixam o formato de cada
    arquivo num lugar só. `rotas.csv` grava a melhor rota só quando ela
    melhora, porque com elitismo ela não muda entre duas melhorias; isso
    basta para reconstruir a solução de qualquer época sem repetir a mesma
    rota milhares de vezes.
  - `io::ArgParser` lê `--chave valor` e aponta as chaves que ninguém
    consultou, para que um erro de digitação (`--mutaçao`) não rode
    calado com o valor padrão.
  - CMake: biblioteca estática `tsp_nucleo` com tudo menos os `main`,
    executáveis `tsp` e `experimentos`, um teste por bloco (`ctest`),
    Release por padrão e `-static` no MinGW.
- **Alternativas consideradas**: GoogleTest ou Catch2 — rejeitados, são
  uma dependência baixada pelo CMake para testes que cabem num
  `verificar(condição, descrição)` (`tests/Verificacao.hpp`). Calcular a
  distância sob demanda — rejeitado, seria uma raiz por aresta, por
  indivíduo, por geração. Gravar a rota de toda época em `rotas.csv` —
  rejeitado pelo tamanho (500 pontos × 5000 épocas) sem informação nova.
- **Consequências**: a matriz custa `8·n²` bytes (cerca de 32 MB com
  2000 pontos), o que limita o tamanho do bônus mas não os cenários
  normais. Quem plota (D) precisa reconstruir a rota de cada época a
  partir das melhorias (`rotas.csv`), o que é um `searchsorted` sobre a
  coluna `geracao`.
- **Testes portados da pessoa-A**: a branch `pessoa-A` tinha outra
  implementação do bloco A, com testes próprios. Na integração ficou esta
  implementação, e os casos daqueles testes que valem para esta API foram
  portados para `tests/core/TesteCore.cpp` e `tests/io/TesteIo.cpp`:
  - matriz inteira conferida contra `distanciaEuclidiana`;
  - geometria do círculo e ótimo com n = 4;
  - 1000 `Individuo` aleatórios nunca abaixo do ótimo;
  - seed controlando o uniforme;
  - erros do `ArgParser`;
  - `pontos.csv` e `resumo.txt` com e sem ótimo;
  - falha de escrita.
  
  Ficaram de fora os que dependiam do contrato antigo: formato dos CSVs,
  `aptidao(0)` lançando erro e chave repetida ou sem valor lançando erro.
- **Data**: 2026-10-21 (testes portados em 2026-10-26)

## 006 — Laço evolutivo e parâmetros: população 400, mutação 0,5, estagnação `max(300, 3·n)`

- **Contexto**: o bloco C precisava fixar os parâmetros do laço (população,
  taxa de mutação, critério de parada) com justificativa medida, não só
  "valores usuais". O ERX (decisão 002) faz a população perder
  diversidade rápido, então a interação com a mutação precisava ser
  medida.
- **Decisão**:
  - Laço geracional com elitismo de 2, torneio k = 3 e cruzamento 0,9.
    Esses três ficaram em valores usuais e não foram variados.
  - Taxa de mutação = probabilidade de um filho sofrer uma inversão
    (por indivíduo, não por gene). **0,5**.
  - População **400**.
  - Parada por estagnação da melhor rota, com tolerância relativa 1e-9,
    limite **`max(300, 3·n)`** (`estagnacaoPadrao` em `Parametros.hpp`),
    mais um teto de segurança de 20 000 gerações.
  - Diversidade (arestas distintas na população / n) registrada a cada
    época, para mostrar o efeito da mutação sobre o ERX.
  - Medições: `experimentos` com 10 seeds sobre a mesma instância, 50 e
    100 pontos, nos dois cenários. Gap do uniforme contra uma referência
    2-opt + Or-opt (`scripts/referencia_2opt.py`), porque o melhor do
    próprio AG seria uma referência fraca. Números e tabelas no README.
- **Alternativas consideradas**:
  - Mutação 0,8 — melhor no uniforme (cerca de 1 ponto percentual), mas falha no
    círculo de 100 pontos em 4 das 5 populações testadas (uma execução
    termina 96% acima do ótimo). Mutação 1,0 falha sempre nesse círculo.
    Sem mutação, a melhor rota para de melhorar em no máximo 59 gerações e
    a população termina inteira igual (diversidade 1,0).
  - População 200 com mais gerações — rejeitada: com o mesmo trabalho
    total, 400 é igual ou melhor, e com 100 pontos esperar até 2000
    gerações paradas não muda o resultado de P = 200. População 800 —
    ainda melhora, mas cada duplicação rende menos e dobra o custo por
    época.
  - Limite fixo de estagnação — rejeitado: o mínimo que o círculo precisa
    para não parar antes do ótimo cresce com n (50, 200 e 400 gerações com
    50, 100 e 200 pontos). Parar ao atingir o ótimo — só existe no círculo.
    Parar quando a população converge — com mutação 0,5 ela não converge
    (diversidade final ~9,6), então nunca dispararia.
- **Consequências**: com 50 e 100 pontos uma execução leva de 2 a 16 s e
  acerta o ótimo do círculo em todas as seeds. No uniforme, o AG puro fica
  a ~2% da referência com 50 pontos e a ~7,5% com 100. Com mais pontos,
  o gap cresce, e o próximo ganho real viria de busca local (2-opt
  memético, decisão 003), não de ajuste fino desses parâmetros. Os tempos
  da varredura em paralelo servem só para comparar configurações entre
  si; o tempo de referência (e do bônus) é o do `tsp` rodando sozinho.
- **Data**: 2026-10-22
