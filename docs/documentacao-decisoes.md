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

## 005 — Implementação do bloco A: `core/`, `io/`, build e testes

- **Contexto**: B, C e D dependem do bloco A (problema, aptidão e
  infraestrutura). O contrato (assinaturas dos `.hpp` e formato dos
  arquivos de saída) precisava ficar fechado antes da integração, e
  algumas escolhas não ficam óbvias só lendo o código.
- **Decisão**:
  - **Matriz de distâncias pré-calculada** (`calcularMatrizDistancias`,
    `std::vector<std::vector<double>>`): calculada uma vez antes do AG;
    cada avaliação de rota vira leitura de memória em vez de `sqrt`.
    Calcula só `i < j` e espelha (simétrica, diagonal zero).
  - **`distanciaRota` recebe `std::vector<int>`**, não
    `genetico::Individuo`: como `Individuo` é só um alias, funciona igual,
    e `core/` não passa a depender de `genetico/` (dependência só no
    sentido A → todos). Soma o **ciclo fechado**, com a aresta
    `rota[N-1] → rota[0]` via `(i + 1) % N`.
  - **Aptidão = 1 / distância**, recebendo a distância já calculada (C usa
    o mesmo valor no histórico sem somar a rota de novo). O `Torneio`
    maximiza e o problema minimiza, então precisa de uma função
    decrescente; como o torneio só compara, qualquer função decrescente
    daria a mesma seleção — 1/d por ser positiva, sem parâmetro e
    invariante à escala. Lança erro para distância ≤ 0.
  - **`Cenario` guarda `std::optional<double> otimoConhecido`**: o círculo
    preenche com `2·N·R·sin(π/N)`, o uniforme fica `std::nullopt`. O
    círculo sai em ordem angular (a rota `0..N-1` é a ótima) e não usa
    rng — a população inicial já é aleatória.
  - **`ArgParser` genérico** (`--chave valor`, `obterInt/Double/String`
    com padrão): não conhece os `Parametros` de C, então C cria ou
    renomeia parâmetros sem mexer em A. Valida a conversão inteira
    (`12abc` é erro) e expõe `chavesNaoUsadas()` para pegar erro de
    digitação (`--geracos`).
  - **Formato dos arquivos** (combinado com D): vírgula como separador,
    ponto decimal (locale "C", independente do Windows em pt-BR), 6 casas
    em `fixed` (sem notação científica). `historico.csv` guarda
    distâncias, não aptidão. Em `rotas.csv` a rota fica numa coluna só,
    cidades separadas por espaço, sem repetir a primeira no fim.
    `resumo.txt` em `chave=valor`, com `otimo` e `gap_percentual` só
    quando há ótimo.
  - **Testes sem framework externo** (`tests/Assercoes.hpp`), um
    executável por módulo registrado no CTest. **Build** em CMake com
    Ninja, C++17, Release por padrão e `-static` no MinGW.
- **Alternativas consideradas**: calcular a distância sob demanda —
  economiza memória O(N²), mas repete `sqrt` milhões de vezes; com N =
  1000 a matriz ocupa só ~8 MB. Matriz achatada (`vector<double>` com
  `i*N + j`) — mais amigável ao cache, mas menos legível; a interface
  permite trocar depois sem afetar ninguém. `ArgParser` preenchendo
  `Parametros` diretamente — acoplaria A a C. Rota em formato longo
  (`epoca,posicao,cidade`) — N linhas por snapshot, arquivo muito maior
  no bônus. GoogleTest/Catch2 — exigiria baixar dependência e poderia
  brigar com o link estático, sem ganho para testes deste tamanho.
- **Consequências**: memória O(N²) limita na prática a uns 5000 pontos
  (~200 MB). Os testes cobrem os invariantes que, se quebrados, fariam o
  AG convergir silenciosamente para a resposta errada (ciclo fechado,
  ótimo de referência, sentido da aptidão, matriz simétrica) e o formato
  exato de cada arquivo. Os executáveis `main.cpp`/`experimentos.cpp`
  estão comentados no `CMakeLists.txt` até C implementá-los. No Windows,
  o build precisa ser feito pelo PowerShell (pelo Git Bash o g++ do
  MSYS2 falha sem mensagem).
- **Data**: 2026-10-23
