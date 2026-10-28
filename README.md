# Algoritmo Genético para resolução do problema do Caixeiro Viajante
**Descrição do problema:**
Dado um conjunto de pontos (em 2D), encontra um ciclo que visita todos os pontos uma vez, minimizando a distância euclidiana percorrida

## O Algoritmo Genético
- **Cromossomos:** Permutação dos índices das cidades
- **Seleção:** Torneio de tamanho k (k = 3)
- **Cruzamento:** ERX (*Edge Recombination Crossover*), taxa 0,9
- **Mutação:** Inversão de um trecho da rota, taxa 0,5 por filho
- **Aptidão:** 1/distânciaTotalCiclo
- **População:** 400 indivíduos, elitismo de 2
- **Parada:** `max(300, 3·n)` gerações sem a melhor rota melhorar, com teto de 20 000

## Função de aptidão
A aptidão de uma rota é `1 / D`, onde `D` é o comprimento do ciclo fechado: a soma das
distâncias euclidianas entre cidades consecutivas, **incluindo a volta da última para a
primeira** (`core::distanciaRota` e `core::aptidao`, em `src/core/Distancia.cpp`).

- **Por que inverter a distância:** o problema é de minimização, mas a seleção escolhe o
  mais apto, ou seja, o maior valor. `1/D` é estritamente decrescente em `D`, então a rota
  mais curta tem sempre a maior aptidão. Também é sempre positiva (`D > 0`), o que permitiria
  trocar o torneio por roleta sem mexer na aptidão.
- **Com torneio, só a ordem importa:** o torneio compara aptidões com `>`, então qualquer
  função decrescente de `D` (`-D`, `1/D`, `1/D²`) produziria exatamente a mesma seleção.
  `1/D` não muda o comportamento: só deixa a aptidão positiva e fácil de interpretar.
- **Por que não roleta:** numa população de rotas parecidas (por exemplo, distâncias entre
  640 e 700), as aptidões `1/640` e `1/700` diferem só 9%, e a roleta quase não
  distinguiria o melhor do pior. O torneio depende só da ordem e mantém a mesma pressão
  seletiva do começo ao fim.
- **Alternativas descartadas:** `Dmax − D` depende da população (a aptidão de uma mesma
  rota mudaria a cada geração) e zera o pior indivíduo. `-D` é negativa e não serve para
  métodos proporcionais.
- **Custo:** as distâncias entre todos os pares ficam numa matriz pré-calculada
  (`core::MatrizDistancias`), então avaliar uma rota custa `n` somas, sem raiz quadrada.

## Cruzamento: ERX (Edge Recombination Crossover)
A maioria das soluções desse problema usa OX (Order Crossover), que preserva blocos de posições dos pais.
Optamos pela utilização do ERX pois, no problema do caixeiro viajante (TSP), o que importa para a qualidade da rota não é a posição da cidade na lista, mas sim quais cidades ficam vizinhas uma da outra. ERX trabalha direto em cima disso ao invés de recombinar posições, recombina arestas, isso é, pares de cidades vizinhas.

### Ideia Geral
1. Monta uma tabela de vizinhos para cada cidade, junta os vizinhos que ela tem no Pai1 e Pai2, cada pai contribui com até 2 vizinhos por cidade, o anterior e o próximo na rota, de forma circular.

2. Começa o filho em uma cidade fixa, a primeira do pai 1

3. A cada passo, olha os vizinhos da cidade atual que ainda não foram visitados. Se algum deles é vizinho nos dois pais (a mesma aresta aparece em pai1 e pai2), restringe a escolha só a esses: são as arestas que vale mais a pena preservar. Dentro do grupo restante (os comuns, ou todos os candidatos se nenhum é comum), escolhe o de menor grau. O "grau" aqui é da cidade candidata, não da aresta: é quantos vizinhos ela ainda tem na tabela depois que a cidade atual é removida da lista de todo mundo (passo 4 explica por quê). Prioriza o de menor grau porque ele é o mais fácil de ficar sem saída mais à frente, então é melhor usá-lo antes que isso aconteça.

4. Se há empate, sorteia entre os empatados. Se não há nenhum vizinho disponível, "beco sem saída", sorteia entre qualquer cidade não visitada

5. Repete até o filho ter todas as cidades

Esse processo sempre produz uma permutação válida direto, sem precisar de reparo depois, diferente do OX

### Exemplo 

Dois pais com 6 cidades
pai1 = [A, B, C, D, E, F]
pai2 = [B, D, A, F, C, E]

Tabela de vizinhos, união de arestas dos dois pais, cada um contribui com o vizinho anterior e o próximo, de forma circular. Em negrito, o vizinho que aparece nos dois pais (aresta comum)
| Cidade | Vizinhos (pai1) | Vizinhos (pai2) | Tabela combinada |
|---|---|---|---|
| A | B, F | D, F | {B, D, **F**} |
| B | A, C | D, E | {A, C, D, E} |
| C | B, D | E, F | {B, D, E, F} |
| D | C, E | A, B | {A, B, C, E} |
| E | D, F | B, C | {B, C, D, F} |
| F | A, E | A, C | {**A**, C, E} |

Caminhada (Começando em A, convenção do pai1)
| Passo | Cidade atual | Candidatos não visitados | Aresta comum aos dois pais? | Escolhido |
|---|---|---|---|---|
| 1 | A | B, D, F | só F | F (único candidato com aresta comum) |
| 2 | F | C, E | nenhum → grau: C:3, **E:3** (empate) | E (sorteio) |
| 3 | E | B, C, D | nenhum → todos grau 2 (empate) | D (sorteio) |
| 4 | D | B, C | nenhum → todos grau 1 (empate) | C (sorteio) |
| 5 | C | B | único candidato | B |

Filho Resultante [A, F, E, D, C, B]

Nota-se que o filho reaproveitou arestas presentes nos pais, em vez de reaproveitar as posições: A-F é comum aos dois pais e por isso foi a primeira escolhida, e E-D vem do pai1. Quando não sobra nenhum vizinho não visitado para a cidade atual, o algoritmo sorteia qualquer cidade pendente, esse é o fallback que evita o algoritmo travar, ao custo de quebrar uma aresta dos pais nesse ponto

### Trade-off
ERX preserva mais a estrutura dos pais do que o OX, mas por isso também converge mais rápido para rotas parecidas entre si, a população perde diversidade mais rápido. Compensamos isso com a taxa de mutação que fica mais alta do que seria necessário com OX

## Laço evolutivo e parâmetros
Cada época (geração) de `AlgoritmoGenetico::executar` (`src/algoritmo/AlgoritmoGenetico.cpp`):

1. Os 2 melhores indivíduos (elite) passam intactos para a próxima geração, então a
   melhor rota nunca piora.
2. O resto da população é preenchido com filhos: dois pais escolhidos por torneio
   (k = 3), cruzados por ERX com probabilidade 0,9 (senão o filho é cópia do pai 1),
   e o filho sofre uma inversão de trecho com probabilidade igual à taxa de mutação.
3. A nova população substitui a antiga inteira (modelo geracional).
4. A execução para quando a melhor rota passa um número de gerações sem melhorar
   (estagnação), ou no teto de gerações.

A população inicial é de permutações uniformemente aleatórias, sem nenhuma heurística
(vizinho mais próximo etc.), para que o resultado venha só da evolução. Cada época
registra melhor, média, pior, desvio e a **diversidade** da população: o número de
arestas distintas presentes em toda a população dividido por `n`. Ela vale 1 quando
todo mundo é a mesma rota e cresce conforme as rotas diferem; é a medida usada abaixo
para mostrar a população convergindo.

| Parâmetro | Padrão | Opção | Justificativa |
|---|---|---|---|
| População | 400 | `--populacao` | [População e critério de parada](#população-e-critério-de-parada) |
| Taxa de mutação | 0,5 | `--mutacao` | [Taxa de mutação](#taxa-de-mutação) |
| Estagnação | `max(300, 3·n)` gerações | `--estagnacao` | [Critério de parada](#critério-de-parada) |
| Teto de gerações | 20000 | `--geracoes` | [Critério de parada](#critério-de-parada) |
| Taxa de cruzamento | 0,9 | `--cruzamento` | valor usual; os 10% de cópias deixam passar rotas boas só com mutação |
| Torneio | 3 | `--torneio` | pressão seletiva moderada; com k maior a população converge ainda mais rápido, o que o ERX já faz sozinho |
| Elite | 2 | `--elite` | o suficiente para a melhor rota nunca se perder, sem ocupar a população |

Taxa de cruzamento, torneio e elite ficaram nos valores usuais e não foram variados.
População, mutação e estagnação foram escolhidas com o `experimentos`: cada
configuração rodou com 10 seeds do AG sobre a mesma instância de pontos, nos dois
cenários, com 50 e 100 pontos. No círculo, o gap é medido contra o ótimo
`2·n·r·sin(π/n)`. No uniforme, que não tem ótimo conhecido, é medido contra a melhor
distância de 300 reinícios de 2-opt + Or-opt (`scripts/referencia_2opt.py`): 616,54
para 50 pontos e 821,64 para 100. Essa busca local acha o ótimo exato do círculo, e
o AG chegou a igualar a de 50 pontos (com população 800). Nas tabelas, o uniforme
aparece como gap médio ± desvio entre as 10 seeds, e o círculo como quantas das 10
execuções atingiram o ótimo exato (com o gap médio entre parênteses).

## População e critério de parada

### População: 400
**O papel da população.** O ERX só recombina arestas que já existem nos pais, então a
população é o estoque de arestas disponível para montar a rota. Com poucos indivíduos
esse estoque se esgota cedo: a população converge antes de juntar as arestas da rota
ótima, e mais gerações não resolvem. Do outro lado, o custo de cada época cresce
linearmente com a população (um ERX de custo `O(n)` por filho).

Varredura com taxa de mutação 0,5 e o critério de parada padrão (10 seeds):

| População | Uniforme 50 | Círculo 50 | Gerações (50) | Uniforme 100 | Círculo 100 | Gerações (100) |
|---|---|---|---|---|---|---|
| 25 | 8,8% ± 5,0 | 10/10 | 891 | — | — | — |
| 50 | 6,7% ± 1,9 | 10/10 | 725 | 10,8% ± 2,6 | 9/10 (2,2%) | 1667 |
| 100 | 6,3% ± 3,3 | 10/10 | 544 | 7,2% ± 1,8 | 10/10 | 1208 |
| 200 | 3,4% ± 1,8 | 10/10 | 474 | 7,7% ± 1,6 | 10/10 | 877 |
| **400** | **2,2% ± 1,2** | **10/10** | **446** | **7,5% ± 2,3** | **10/10** | **734** |
| 800 | 1,5% ± 1,3 | 10/10 | 423 | 6,5% ± 2,0 | 10/10 | 641 |

População maior dá rota melhor no uniforme e precisa de menos gerações para parar, mas
cada geração custa proporcionalmente mais. Para saber se vale mais gastar o trabalho em
população ou em gerações, comparamos pelo **trabalho total** (filhos gerados =
população × gerações), usando os pontos de parada do experimento de estagnação (abaixo)
para dar mais gerações à mesma execução:

| Configuração | Filhos (50 pontos) | Uniforme 50 | Filhos (100 pontos) | Uniforme 100 |
|---|---|---|---|---|
| P = 200, estagnação 300 | 103 mil | 3,39% | 170 mil | 7,67% |
| P = 200, estagnação 1000 | 274 mil | 2,85% | 324 mil | 7,56% |
| P = 200, estagnação 2000 | 530 mil | 2,70% | 524 mil | 7,56% |
| **P = 400, estagnação 300** | **195 mil** | **2,23%** | **293 mil** | **7,55%** |
| P = 400, estagnação 1000 | 577 mil | 1,43% | 637 mil | 7,09% |
| P = 400, estagnação 2000 | 1155 mil | 1,02% | 1546 mil | 5,79% |

Com 200 indivíduos, esperar mais não compensa: com 100 pontos, ir de 300 para 2000
gerações de estagnação triplica o trabalho e o resultado fica praticamente igual
(7,67% → 7,56%), porque a população já perdeu as arestas que faltavam. Com 400, o
padrão faz menos trabalho que P = 200 com estagnação 1000 e chega a um resultado melhor
(50 pontos) ou igual (100 pontos).

**Escolha.** Dobrar a população continua rendendo mais que esperar mais gerações também
acima de 400: P = 800 com o padrão chega a 1,5% com 338 mil filhos, contra 1,43% de
P = 400 com estagnação 1000 e 577 mil filhos. Mas o ganho de cada duplicação vem caindo
(no uniforme de 50 pontos, 200 → 400 tira 1,2 ponto percentual e 400 → 800 tira 0,7)
enquanto o custo por época dobra. 400 é o compromisso: acerta o ótimo do círculo em
todas as execuções nos dois tamanhos, fica a cerca de 2% da referência no uniforme de
50 pontos, e uma execução leva poucos segundos (tabela abaixo). Isso deixa margem para
o bônus com muitos pontos, onde o custo por época cresce com `n`.

### Critério de parada
Dois critérios, o que vier primeiro:

- **Estagnação:** para quando a melhor rota passa `max(300, 3·n)` gerações sem
  melhorar (300 até 100 pontos, 600 com 200, 1500 com 500). "Melhorar" usa tolerância
  relativa de 1e-9, para que a mesma rota começando em outra cidade, que soma as mesmas
  arestas em outra ordem e pode diferir no último bit, não conte como melhora.
- **Teto de 20 000 gerações:** só uma rede de segurança. Com os padrões, a execução
  mais longa dos experimentos parou em 676 gerações com 50 pontos, 802 com 100 e
  2465 com 200.

**Por que estagnação, e não um número fixo de gerações.** A geração da última melhora
varia muito entre execuções. Com mutação 0,5, ela foi de 86 a 1071 com 50 pontos e de
281 a 1950 com 100. Um número fixo ou desperdiça tempo nas execuções que já convergiram,
ou corta as que ainda estão melhorando.

**Por que não parar ao atingir o ótimo.** Só o círculo tem ótimo conhecido. Usá-lo para
parar daria ao AG uma informação que ele não tem no uniforme; o ótimo serve só para
medir o gap.

**Por que não parar quando a população converge** (média igual ao melhor, ou
diversidade chegando a 1). Com mutação 0,5 a população nunca converge: a diversidade
final fica em torno de 9,6 com 400 indivíduos, e a média fica sempre acima do melhor.
Um critério assim nunca dispararia. O que importa é se a melhor rota ainda melhora.

**Como o limite foi escolhido.** O modo `estagnacao` do `experimentos` roda cada seed
sem parada antecipada (10 000 gerações; 15 000 com 200 pontos) e calcula, para cada
limite, em que geração aquela execução teria parado e com qual rota. Isso é exato, e
não uma estimativa: o sorteio não depende do critério de parada, então a execução com
limite S é idêntica à longa até o ponto de parada (conferido: com limite 300 as 40
execuções coincidem com as da varredura). A perda é quanto a rota no ponto de parada
fica acima da melhor rota da execução longa. Com população 400 e mutação 0,5:

| Limite | Círculo 50 | Círculo 100 | Círculo 200 | Uniforme 50 | Uniforme 100 |
|---|---|---|---|---|---|
| 25 | 9/10 · 124 | 3/10 · 394 | — | 2,27% · 145 | 2,63% · 376 |
| 50 | 10/10 · 153 | 5/10 · 439 | — | 2,27% · 170 | 2,33% · 428 |
| 100 | 10/10 · 203 | 9/10 · 526 | 8/10 · 1573 | 2,03% · 228 | 2,04% · 531 |
| 200 | 10/10 · 303 | 10/10 · 637 | 8/10 · 1673 | 1,64% · 366 | 2,04% · 631 |
| **300** | 10/10 · 403 | **10/10 · 737** | 9/10 · 1794 | **1,55% · 489** | **2,04% · 731** |
| 400 | — | — | 10/10 · 1929 | — | — |
| 600 | — | — | **10/10 · 2129** | — | — |
| 1000 | 10/10 · 1103 | 10/10 · 1437 | 10/10 · 2529 | 0,75% · 1442 | 1,61% · 1593 |
| 2000 | 10/10 · 2103 | 10/10 · 2437 | 10/10 · 3529 | 0,34% · 2888 | 0,37% · 3865 |

No círculo, cada célula mostra quantas execuções pararam sem perda (todas as execuções
longas chegaram ao ótimo exato) e a geração média de parada. No uniforme, mostra a
perda média e a geração média de parada.

- **O limite mínimo do círculo cresce com `n`:** 50 gerações bastam com 50 pontos, 200
  com 100 e 400 com 200, ou seja, cerca de `2·n`. Com mais cidades, cada melhora fica mais
  rara, porque a inversão certa tem chance da ordem de `1/n²`.
- **No uniforme a perda cai devagar.** Com 100 pontos, passar de 300 para 1000
  gerações só tira 0,4 ponto percentual. O AG ainda acha melhoras pequenas e raras
  depois de centenas de gerações paradas, e esse ganho sai mais barato aumentando a
  população (ver a tabela de trabalho total acima).
- **Escolha:** 300 cobre o círculo até 100 pontos com folga de 1,5× sobre o mínimo
  (200), e `3·n` mantém a mesma folga acima disso. Com 200 pontos, o limite vira 600, e
  as 10 execuções pararam no ótimo exato.

Tempo de uma execução com os padrões (seed 42, uma execução por vez):

| Cenário | Melhor distância | Gerações | Última melhora | Tempo total |
|---|---|---|---|---|
| Uniforme, 50 pontos | 631,35 (2,4% acima da referência) | 433 | geração 133 (0,9 s) | 2,6 s |
| Círculo, 50 pontos | 313,95 (ótimo) | 404 | geração 104 (0,8 s) | 2,3 s |
| Uniforme, 100 pontos | 888,07 (8,1% acima da referência) | 724 | geração 424 (8,2 s) | 14,6 s |
| Círculo, 100 pontos | 314,11 (ótimo) | 783 | geração 483 (9,4 s) | 16,0 s |
| Círculo, 200 pontos | 314,15 (ótimo) | 1917 | geração 1317 (60,6 s) | 87,0 s |

O tempo por época cresce com `população × n`: cerca de 6 ms com 50 pontos, 20 ms com
100 e 45 ms com 200.

## Taxa de mutação
**Padrão: 0,5**, a probabilidade de cada filho sofrer **uma** inversão de trecho.

**O que a taxa mede.** É uma taxa por indivíduo, não por gene: numa permutação não
dá para trocar uma cidade sozinha sem repetir outra, então a mutação age na rota
(inverte o trecho entre duas posições sorteadas). Uma inversão troca exatamente 2 das
`n` arestas da rota. Com taxa 0,5 e 50 pontos, um filho tem em média 0,5 × 2/50 = 2%
das arestas trocadas: a taxa por indivíduo é alta, mas cada mutação perturba pouco.

**Por que tão alta.** O ERX monta o filho só com arestas que existem nos pais, e
prioriza as que estão nos dois (ver o Trade-off do ERX). Arestas novas só entram pela
mutação e pelo sorteio em beco sem saída. Sem mutação, nos 100 testes com 50 pontos,
a melhor rota parou de melhorar em no máximo 59 gerações e a população terminou
inteira igual (diversidade 1,0), longe do ótimo: mesmo com 400 indivíduos, 8,5% acima
da referência no uniforme e nenhum acerto no círculo.

**Por que não mais alta.** Uma inversão aleatória troca duas arestas curtas por duas
cordas entre pontos sorteados, e numa rota já boa isso quase sempre piora. No círculo
de raio 50 com 100 pontos, uma aresta da rota ótima mede 3,1 e uma corda aleatória
mede em média 64 (`4r/π`). Com taxa alta, a maioria dos filhos nasce estragada e a
seleção trabalha com poucos indivíduos bons. Com 0,8, uma das execuções do círculo de
100 pontos termina com a média da população em 922 e o melhor em 616 (96% acima do
ótimo), preso numa rota em que arcos do círculo são ligados por cordas que atravessam
o centro. Desfazer isso exige sortear exatamente o par de arestas certo, com chance
da ordem de `1/n²` por mutação.

Varredura com população 400 (critério de parada padrão, 10 seeds):

| Taxa | Uniforme 50 | Círculo 50 | Diversidade final (50) | Uniforme 100 | Círculo 100 |
|---|---|---|---|---|---|
| 0 | 8,5% ± 3,8 | 0/10 (31,8%) | 1,0 | — | — |
| 0,05 | 3,8% ± 1,7 | 10/10 | 1,8 | — | — |
| 0,1 | 3,0% ± 1,3 | 10/10 | 2,5 | 7,9% ± 1,8 | 10/10 |
| 0,2 | 3,3% ± 0,9 | 10/10 | 4,1 | — | — |
| 0,3 | 2,8% ± 1,9 | 10/10 | 5,6 | 7,3% ± 2,0 | 10/10 |
| **0,5** | **2,2% ± 1,2** | **10/10** | **9,6** | **7,5% ± 2,3** | **10/10** |
| 0,8 | 1,5% ± 1,0 | 10/10 | 16,2 | 5,8% ± 3,1 | 7/10 (10,2%) |
| 1,0 | 3,7% ± 1,5 | 9/10 (0,4%) | 19,8 | 8,7% ± 2,7 | 0/10 (25,4%) |

Execuções do círculo de 100 pontos que atingiram o ótimo, por população:

| Taxa | P = 50 | 100 | 200 | 400 | 800 |
|---|---|---|---|---|---|
| 0,1 | 0/10 | 2/10 | 5/10 | 10/10 | 10/10 |
| 0,3 | 3/10 | 9/10 | 10/10 | 10/10 | 10/10 |
| **0,5** | 9/10 | **10/10** | **10/10** | **10/10** | **10/10** |
| 0,8 | 4/10 | 5/10 | 10/10 | 7/10 | 6/10 |
| 1,0 | 0/10 | 0/10 | 0/10 | 0/10 | 0/10 |

**Escolha.** 0,8 dá o melhor resultado no uniforme, mas não é confiável no círculo de
100 pontos. 0,5 é a maior taxa que acertou o ótimo do círculo em todas as execuções
com população ≥ 100, nos dois tamanhos, e perde menos de 2 pontos percentuais para
0,8 no uniforme. Como o círculo é o benchmark com ótimo conhecido, e o bônus é um
círculo com muitos pontos (onde acertar o par certo de arestas fica ainda mais raro),
preferimos a taxa confiável. Isso confirma o que o Trade-off do ERX antecipava: com ERX
a mutação precisa ficar alta, mas não no máximo.

## Referência
- Whitley, D., Starkweather, T., & Fuquay, D. (1989). *Scheduling problems
  and traveling salesmen: The genetic edge recombination operator.*

## Como Rodar

### Requisitos
- C++17 com CMake e Ninja. No Windows, o MSYS2 UCRT64 já tem tudo:
  `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja`.
  O `C:\MinGW` antigo (GCC 6.3) **não serve**: ele não tem C++17 (`<optional>`, structured bindings).
- Os executáveis são linkados com `-static`, então rodam fora do terminal do MSYS2.

### Compilar e testar (PowerShell)
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"   # só se o MSYS2 não estiver no PATH
cmake -S . -B build -G Ninja                      # Release por padrão
cmake --build build
ctest --test-dir build --output-on-failure        # testes de core, io e algoritmo
```

### Executar um cenário
```powershell
.\build\tsp.exe --cenario uniforme --n 50
.\build\tsp.exe --cenario circulo --n 50 --saida saidas/circulo
.\build\tsp.exe --ajuda                           # todas as opções e os valores padrão
```
O terminal mostra uma linha a cada `--log-cada` épocas (melhor, média, diversidade,
gap ao ótimo no círculo e tempo, com `*` quando a melhor rota melhorou). Na pasta
`--saida` (padrão `saidas/<cenario>`) ficam:

| Arquivo | Conteúdo |
|---|---|
| `pontos.csv` | `indice,x,y` de cada cidade |
| `historico.csv` | `geracao,melhor,media,pior,desvio,diversidade,tempo_ms`, uma linha por época |
| `rotas.csv` | `geracao,distancia,tempo_ms,rota`, uma linha cada vez que a melhor rota melhorou (com elitismo, a melhor rota de qualquer época é a última linha com `geracao` ≤ época) |
| `resumo.txt` | parâmetros, melhor distância, gap, gerações, motivo da parada e tempos |

### Experimentos de parâmetros
```powershell
.\build\experimentos.exe --modo ambos --n 50 --seeds 10
```
Gera `varredura.csv`/`resumo_varredura.csv` (população × taxa de mutação) e
`estagnacao.csv`/`resumo_estagnacao.csv` (limite de estagnação) em
`saidas/experimentos`, e imprime as tabelas resumidas. `--populacoes`, `--mutacoes`,
`--estagnacoes`, `--cenarios` e `--threads` mudam a grade. Os tempos medidos aqui são
de execuções simultâneas (uma por thread) e só servem para comparar configurações
entre si.

As tabelas das seções acima saíram destes comandos. A varredura rodou com o teto
antigo de 5000 gerações, então `--geracoes 5000` reproduz os mesmos números (algumas
execuções com mutação 1,0 e 100 pontos chegam nesse teto). Cada um leva de minutos a
algumas horas com 20 threads:
```powershell
.\build\experimentos.exe --modo varredura --n 50 --geracoes 5000
.\build\experimentos.exe --modo varredura --n 50 --geracoes 5000 --populacoes 800 --mutacoes 0.3,0.5,0.8,1
.\build\experimentos.exe --modo varredura --n 100 --geracoes 5000 --populacoes 50,100,200,400,800 --mutacoes 0.1,0.3,0.5,0.8,1
.\build\experimentos.exe --modo estagnacao --n 50 --populacao 200      # e --populacao 400; idem com --n 100
.\build\experimentos.exe --modo estagnacao --n 200 --cenarios circulo --geracoes-longas 15000 --estagnacoes 100,200,300,400,600,800,1000,1500,2000,3000
python scripts/referencia_2opt.py saidas/uniforme/pontos.csv          # referência do uniforme
```