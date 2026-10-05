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
