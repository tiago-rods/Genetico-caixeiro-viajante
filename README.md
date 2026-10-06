# Algoritmo Genético para resolução do problema do Caixeiro Viajante
**Descrição do problema:**
Dado um conjunto de pontos (em 2D), encontra um ciclo que visita todos os pontos uma vez, minimizando a distância euclidiana percorrida

## O Algoritmo Genético
**Cromossomos:** Permutação dos índices das cidades
**Seleção:** Torneio de tamanho k
**Cruzamento:** ERX (*Edge Recombination Crossover*)
**Mutação:** Inversão de um trecho da rota
**Aptidão:** 1/distânciaTotalCiclo


## Cruzamento: ERX (Edge Recombination Crossover)
A maioria das soluções desse problema usa OX (Order Crossover), que preserva blocos de posições dos pais.
Optamos pela utilização do ERX pois, no problema do caixeiro viajante (TSP), o que importa para a qualidade da rota não é a posição da cidade na lista, mas sim quais cidades ficam vizinhas uma da outra. ERX trabalha direto em cima disso ao invés de recombinar posições, recombina arestas, isso é, pares de cidades vizinhas.

### Ideia Geral
1. Monta uma tabela de vizinhos para cada cidade, junta os vizinhos que ela tem no Pai1 e Pai2, cada pai contribui com até 2 vizinhos por cidade, o anterior e o próximo na rota, de forma circular.

2. Começa o filho em uma cidade fixa, a primeira do pai 1

3. A cada passo, olha os vizinhos da cidade atual que ainda não foram visitados e escolhe o de menor grau restante. O "grau" aqui é da cidade candidata, não da aresta: é quantos vizinhos ela ainda tem na tabela depois que a cidade atual é removida da lista de todo mundo (passo 4 explica por quê). Prioriza o de menor grau porque ele é o mais fácil de ficar sem saída mais à frente, então é melhor usá-lo antes que isso aconteça.

4. Se há empate, sorteia entre os empatados. Se não há nenhum vizinho disponível, "beco sem saída", sorteia entre qualquer cidade não visitada

5. Repete até o filho ter todas as cidades

Esse processo sempre produz uma permutação válida direto, sem precisar de reparo depois, diferente do OX

### Exemplo 

Dois pais com 6 cidades
pai1 = [A, B, C, D, E, F]
pai2 = [B, D, A, F, C, E]

Tabela de vizinhos, união de arestas dos dois pais, cada um contribui com o vizinho anterior e o próximo, de forma circular
| Cidade | Vizinhos (pai1) | Vizinhos (pai2) | Tabela combinada |
|---|---|---|---|
| A | B, F | D, F | {B, D, F} |
| B | A, C | D, E | {A, C, D, E} |
| C | B, D | E, F | {B, D, E, F} |
| D | C, E | A, B | {A, B, C, E} |
| E | D, F | B, C | {B, C, D, F} |
| F | A, E | A, C | {A, C, E} |

Caminhada (Começando em A, convenção do pai1)
| Passo | Cidade atual | Candidatos não visitados | Grau de cada candidato | Escolhido |
|---|---|---|---|---|
| 1 | A | B, D, F | B:3, D:3, **F:2** | F (menor grau) |
| 2 | F | C, E | C:3, **E:3** (empate) | E (sorteio) |
| 3 | E | B, C, D | todos grau 2 (empate) | D (sorteio) |
| 4 | D | B, C | todos grau 1 (empate) | C (sorteio) |
| 5 | C | B | único candidato | B |

Filho Resultante [A, F, E, D, C, B]

Nota-se que o filho reaproveitou arestas presentes nos pais: A-F vem do pai2 e E-D vem do pai1, em vez de reaproveitar as posições. Quando não sobra nenhum vizinho não visitado para a cidade atual, o algoritmo sorteia qualquer cidade pendente, esse é o fallback que evita o algoritmo travar, ao custo de quebrar uma aresta dos pais nesse ponto

### Trade-off
ERX preserva mais a estrutura dos pais do que o OX, mas por isso também converge mais rápido para rotas parecidas entre si, a população perde diversidade mais rápido. Compensamos isso com a taxa de mutação que fica mais alta do que seria necessário com OX

## Referência
- Whitley, D., Starkweather, T., & Fuquay, D. (1989). *Scheduling problems
  and traveling salesmen: The genetic edge recombination operator.*

## Como Rodar
TODO