# Algoritmo genético para o caixeiro viajante — relatório

Núcleo em C++17 (`build/tsp.exe` e `build/experimentos.exe`), gráficos em Python
(`viz/`). Todas as figuras saem de `scripts/rodar_tudo.ps1`, que as copia para
`docs/figuras/`; as tabelas das seções 2 e 3 vêm das varreduras da Pessoa C
(`docs/experimentos/`). Os detalhes de cada escolha estão no `README.md` e em
`docs/documentacao-decisoes.md`.

## 1. Problema e cenários

Dado um conjunto de N pontos no plano (N ≥ 8), encontrar o ciclo que visita todos uma
vez e volta ao início com a menor distância euclidiana total.

- **Uniforme**: N pontos sorteados em `[0, 100]²` (`--seed-pontos`, padrão 42). A
  seed do AG (`--seed`) não muda os pontos, então as várias seeds de um cenário medem
  a variação do AG sobre a mesma instância.
- **Círculo** (benchmark): N pontos igualmente espaçados num círculo de raio 50
  centrado em `(50, 50)`. O ótimo é conhecido: `2·N·r·sin(π/N)`. Os pontos saem em
  ordem angular, mas a população inicial é embaralhada e nenhum operador olha o valor
  do índice, então isso não ajuda o AG.
- As distâncias entre todos os pares ficam numa matriz pré-calculada; avaliar uma rota
  custa N somas.

Referências usadas nos gráficos (`viz/referencias.py`, calculadas a partir do
`pontos.csv`):

| Referência | Onde | Como |
|---|---|---|
| Ótimo | círculo | fórmula acima (vem no `resumo.txt`) |
| Força bruta | N ≤ 10 | ótimo exato por programação dinâmica (Held-Karp) |
| Referência 2-opt | uniforme, N > 10 | 2-opt + Or-opt com 300 reinícios (`scripts/referencia_2opt.py`); não é o ótimo garantido, mas acha o ótimo exato do círculo |
| Vizinho mais próximo | todos | heurística gulosa, a melhor entre todas as cidades de partida |

## 2. População e critério de parada (1 ponto)

Laço geracional (`AlgoritmoGenetico::executar`): os 2 melhores passam intactos
(elitismo), o resto da população é refeito com filhos (torneio k = 3, ERX com
probabilidade 0,9, senão cópia do pai 1, e mutação), e a nova população substitui a
antiga inteira. A população inicial é de permutações aleatórias, sem heurística.

**População: 400.** O ERX só recombina arestas que já existem nos pais, então a
população é o estoque de arestas disponível; com poucos indivíduos ele se esgota antes
de a rota ótima se formar. Varredura com mutação 0,5 e 10 seeds (gap médio no uniforme,
contra a referência 2-opt; acertos do ótimo no círculo):

| População | Uniforme 50 | Círculo 50 | Uniforme 100 | Círculo 100 |
|---|---|---|---|---|
| 25 | 8,8% ± 5,0 | 10/10 | — | — |
| 50 | 6,7% ± 1,9 | 10/10 | 10,8% ± 2,6 | 9/10 (2,2%) |
| 100 | 6,3% ± 3,3 | 10/10 | 7,2% ± 1,8 | 10/10 |
| 200 | 3,4% ± 1,8 | 10/10 | 7,7% ± 1,6 | 10/10 |
| **400** | **2,2% ± 1,2** | **10/10** | **7,5% ± 2,3** | **10/10** |
| 800 | 1,5% ± 1,3 | 10/10 | 6,5% ± 2,0 | 10/10 |

![Gap de cada seed por população, uniforme](figuras/experimentos/populacao_seeds.png)

Cada duplicação da população rende menos (no uniforme de 50 pontos, 200 → 400 tira 1,2
ponto percentual e 400 → 800 tira 0,7) e dobra o custo por época. Com o mesmo trabalho
total (filhos gerados), 400 indivíduos com o critério padrão chegam a um resultado igual
ou melhor que 200 indivíduos esperando mais gerações. 400 acerta o ótimo do círculo em
todas as execuções com 50 e 100 pontos e deixa uma execução em poucos segundos.

**Critério de parada:** o primeiro entre

- **estagnação**: a melhor rota passa `max(300, 3·N)` gerações sem melhorar
  (tolerância relativa 1e-9);
- **teto de 20 000 gerações**, só como rede de segurança.

A geração da última melhora varia muito entre execuções (de 86 a 1071 com 50 pontos),
então um número fixo de gerações ou desperdiça tempo ou corta execuções que ainda
melhoram. Parar no ótimo só seria possível no círculo, e parar quando a população
converge nunca dispararia: com mutação 0,5 a diversidade não chega a 1. O limite foi
escolhido rodando cada seed sem parada antecipada e calculando onde ela teria parado
com cada limite:

![Perda e acertos por limite de estagnação](figuras/experimentos/estagnacao.png)

No círculo, o limite mínimo para não parar antes do ótimo cresce com N (50 gerações com
50 pontos, 200 com 100, 400 com 200). 300 cobre até 100 pontos com folga de 1,5×, e
`3·N` mantém essa folga acima disso. A linha pontilhada da curva abaixo marca a última
melhora; o que vem depois é o critério esperando a estagnação se confirmar.

![Curva do círculo com N = 50](figuras/circulo_n50_s1/curva.png)

## 3. Taxa de mutação (1 ponto)

A mutação é a probabilidade de cada filho sofrer **uma** inversão de trecho (2-opt
aleatório: troca 2 das N arestas). Valor escolhido: **0,5**.

- **Por que alta:** o ERX monta o filho só com arestas dos pais; arestas novas só entram
  pela mutação. Sem mutação, a melhor rota para de melhorar em no máximo 59 gerações e a
  população termina inteira igual (diversidade 1,0), 8,5% acima da referência no
  uniforme e sem nenhum acerto no círculo.
- **Por que não mais alta:** numa rota boa, uma inversão aleatória quase sempre troca
  duas arestas curtas por duas cordas longas. Com 0,8 ou 1,0 a maioria dos filhos nasce
  estragada; no círculo de 100 pontos, 1,0 não acerta o ótimo em nenhuma execução.

![Mapa de calor população × mutação, uniforme N = 50](figuras/experimentos/mapa_calor_uniforme_n50.png)

![Mapa de calor população × mutação, círculo N = 100](figuras/experimentos/mapa_calor_circulo_n100.png)

0,8 dá o melhor resultado no uniforme, mas falha no círculo de 100 pontos (7/10 com
população 400). 0,5 é a maior taxa que acertou o ótimo do círculo em todas as execuções
com população ≥ 100, nos dois tamanhos. A mesma execução com mutação 0, 0,5 e 1:

![Mutação baixa, padrão e alta no círculo N = 100](figuras/mutacao_circulo_n100.png)

- **Mutação 0:** a diversidade cai para 1 (a população inteira vira a mesma rota) por
  volta da época 100, e o AG trava 179% acima do ótimo.
- **Mutação 0,5:** a diversidade se estabiliza em torno de 6 e o AG chega ao ótimo exato
  na época 411.
- **Mutação 1:** a diversidade fica alta (~18), mas quase todo filho nasce estragado. O
  AG melhora devagar e para na época 2941, 4% acima do ótimo.

## 4. Representação do gene e cruzamento (2 pontos)

- **Gene:** permutação dos índices das cidades. Toda permutação é uma rota válida.
- **Seleção:** torneio de tamanho 3, com reposição.
- **Mutação:** inversão de um trecho entre duas posições sorteadas (sempre diferentes).
- **Cruzamento: ERX** (*Edge Recombination Crossover*). No caixeiro viajante o que
  importa é quais cidades ficam vizinhas, não a posição de cada uma na lista. O ERX
  recombina arestas:
  1. monta, para cada cidade, a tabela dos vizinhos que ela tem nos dois pais (até 4);
  2. começa o filho na primeira cidade do pai 1;
  3. a cada passo, vai para o vizinho ainda não visitado com menos vizinhos restantes
     (o que corre mais risco de ficar sem saída), sorteando nos empates;
  4. se não sobra vizinho (beco sem saída), sorteia uma cidade não visitada.

  O filho é sempre uma permutação válida, sem reparo.

Exemplo com `pai1 = [A B C D E F]` e `pai2 = [B D A F C E]`:

| Cidade | Vizinhos no pai 1 | Vizinhos no pai 2 | Tabela |
|---|---|---|---|
| A | B, F | D, F | {B, D, F} |
| B | A, C | D, E | {A, C, D, E} |
| C | B, D | E, F | {B, D, E, F} |
| D | C, E | A, B | {A, B, C, E} |
| E | D, F | B, C | {B, C, D, F} |
| F | A, E | A, C | {A, C, E} |

Partindo de A: F (menor grau), E (sorteio), D, C, B → filho `[A F E D C B]`, que
reaproveita arestas dos pais (A–F do pai 2, E–D do pai 1).

**Trade-off:** o ERX preserva mais a estrutura dos pais, mas a população fica parecida
mais rápido; por isso a mutação precisa ser alta (seção 3).

**Comparação ERX × OX:** não foi feita. O OX (*Order Crossover*), que copia um trecho do
pai 1 e completa com as cidades que faltam na ordem do pai 2, não foi implementado;
usamos apenas o ERX, pelo argumento acima (o que importa é quais cidades ficam vizinhas,
não a posição de cada uma). O `viz/plotar_experimentos.py` gera `erx_vs_ox.png` sozinho
se um segundo cruzamento aparecer nos CSVs.

## 5. Função de aptidão (1 ponto)

A aptidão de uma rota é `1 / D`, onde `D` é o comprimento do ciclo fechado, incluindo a
volta da última cidade para a primeira (`core::aptidao`).

- O problema é de minimização e a seleção escolhe o maior valor; `1/D` é estritamente
  decrescente em `D`, então a rota mais curta tem a maior aptidão, e é sempre positiva.
- Com torneio só a ordem importa: `-D`, `1/D` ou `1/D²` dariam a mesma seleção.
- Roleta foi descartada: numa população de rotas parecidas (640 a 700), `1/640` e
  `1/700` diferem só 9%, e ela quase não distinguiria o melhor do pior.

## 6. Desempenho por época — cenário aleatório (2,5 pontos)

Parâmetros padrão (população 400, mutação 0,5, estagnação `max(300, 3·N)`), pontos de
`--seed-pontos 42`. As figuras são da seed 1; a tabela do fim da seção usa 10 seeds.

As curvas mostram, por época, a melhor, a média e a pior distância da população, e
embaixo a **diversidade** (arestas distintas na população / N; vale 1 quando todos os
indivíduos são a mesma rota). A linha pontilhada marca a última melhora.

### N = 10: comparação com a força bruta

![Curva, uniforme N = 10](figuras/uniforme_n10_s1/curva.png)

![Rotas, uniforme N = 10](figuras/uniforme_n10_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/uniforme_n10_s1/evolucao.gif)

Com 10 pontos há 9!/2 = 181 440 rotas distintas, e a força bruta dá o ótimo exato
(309,94). O AG chega nele nas 10 seeds, em média na 4ª época; o resto da execução é o
critério de parada esperando 300 épocas sem melhora. O vizinho mais próximo fica 3,0%
acima (319,22).

### N = 50: comparação com o vizinho mais próximo

![Curva, uniforme N = 50](figuras/uniforme_n50_s1/curva.png)

![Gap para a referência 2-opt, uniforme N = 50](figuras/uniforme_n50_s1/gap.png)

![Rotas, uniforme N = 50](figuras/uniforme_n50_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/uniforme_n50_s1/evolucao.gif)

Com 50 pontos não há ótimo conhecido. O AG termina 14,0% ± 1,0 abaixo do vizinho mais
próximo (732,49) e 2,2% ± 1,2 acima da referência 2-opt (616,54), sem igualá-la em
nenhuma seed. A melhora rápida das primeiras ~100 épocas coincide com a queda da
diversidade: o ERX junta as boas arestas da população, e depois disso só a mutação traz
arestas novas, então as melhoras ficam raras e pequenas.

## 7. Desempenho por época — cenário circular (2,5 pontos)

No círculo o ótimo é conhecido, então o gap mede exatamente quanto falta.

### N = 10

![Curva, círculo N = 10](figuras/circulo_n10_s1/curva.png)

![Rotas, círculo N = 10](figuras/circulo_n10_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/circulo_n10_s1/evolucao.gif)

### N = 50

![Curva, círculo N = 50](figuras/circulo_n50_s1/curva.png)

![Gap para o ótimo, círculo N = 50](figuras/circulo_n50_s1/gap.png)

![Rotas, círculo N = 50](figuras/circulo_n50_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/circulo_n50_s1/evolucao.gif)

A grade mostra a rota se desembaraçando: na época 0 ela é um emaranhado de cordas (gap
de 718%). As cordas mais longas somem primeiro, e no fim restam só trechos fora de
ordem, que uma inversão corrige. O AG chega ao ótimo exato (313,95) nas 10 seeds, em
média na época 103 ± 17.

No círculo, o vizinho mais próximo já acha o ótimo, porque o ponto mais perto é sempre
o vizinho no círculo. O AG não usa a geometria (a população inicial é aleatória e os
operadores só veem a permutação), então o círculo mede só a busca do AG.

### Variação entre 10 seeds

| Cenário | N | Melhor distância | Referência | Gap (%) | Atingiu a referência | Vizinho mais próximo | AG vs. vizinho (%) | Gerações | Última melhora | Tempo (s) |
|---|---|---|---|---|---|---|---|---|---|---|
| Uniforme | 10 | 309,94 ± 0,00 | 309,94 (força bruta) | 0,00 ± 0,00 | 10/10 | 319,22 | −2,9 ± 0,0 | 304 ± 2 | 4 ± 2 | 0,84 ± 0,09 |
| Uniforme | 50 | 630,27 ± 7,54 | 616,54 (2-opt) | 2,23 ± 1,22 | 0/10 | 732,49 | −14,0 ± 1,0 | 489 ± 103 | 189 ± 103 | 6,69 ± 1,06 |
| Círculo | 10 | 309,02 ± 0,00 | 309,02 (ótimo) | 0,00 ± 0,00 | 10/10 | 309,02 | 0,0 ± 0,0 | 304 ± 1 | 4 ± 1 | 0,80 ± 0,07 |
| Círculo | 50 | 313,95 ± 0,00 | 313,95 (ótimo) | 0,00 ± 0,00 | 10/10 | 313,95 | 0,0 ± 0,0 | 403 ± 17 | 103 ± 17 | 5,22 ± 1,34 |

Média ± desvio-padrão amostral das seeds 1 a 10 do AG (os pontos são os mesmos em todas;
gerado por `viz/tabela_seeds.py`, em `figuras/media_desvio.md`). Os 2,2% do uniforme com
50 pontos coincidem com a varredura da seção 2, que usou outras seeds.

## 8. Bônus: círculo com N grande (até 1 ponto)

Uma execução por tamanho (seed 1), com os parâmetros padrão e uma execução por vez, para
o tempo não ser dividido com outra. Outros programas estavam abertos na máquina, então os
tempos valem como ordem de grandeza. Valores de `figuras/circulo_n<N>_s1/marcos.txt`.

| N | Épocas até gap ≤ 5% | Tempo (s) | Épocas até gap ≤ 1% | Tempo (s) | Última melhora (época) | Tempo (s) | Épocas no total | Tempo total (s) | Gap final (%) |
|---|---|---|---|---|---|---|---|---|---|
| 100 | 386 | 6,9 | 411 | 7,3 | 411 | 7,3 | 711 | 11,7 | 0,00 |
| 200 | 1781 | 58,9 | 1781 | 58,9 | 1781 | 58,9 | 2381 | 78,2 | 0,00 |
| 500 | 9830 | 1037,4 | não atingiu | — | 9830 | 1037,4 | 11 330 | 1171,3 | 3,20 |

![Convergência, círculo N = 500](figuras/circulo_n500_s1/gap.png)

![Rotas intermediárias, círculo N = 500](figuras/circulo_n500_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/circulo_n500_s1/evolucao.gif). As figuras de 100 e 200
pontos estão em `figuras/circulo_n100_s1/` e `figuras/circulo_n200_s1/`.

- Com 100 e 200 pontos o AG chega ao ótimo exato. Com 200, o gap cai de 44% direto para
  0 numa única melhora (época 1781), por isso os três marcos coincidem.
- Com 500 pontos ele termina 3,2% acima do ótimo, depois de 19,5 min. A curva tem um
  patamar longo perto de 100% (épocas ~4600 a 8300). Nele a rota não dá a volta no
  círculo: vai num sentido pegando pontos alternados e volta pelos que pulou, o que
  mede cerca de duas circunferências. Medido pelo ângulo acumulado, essa rota dá 0
  voltas, e a final dá 1. Sair desse patamar exige recombinar a ordem de muitos
  pontos ao mesmo tempo, o que é raro com inversões aleatórias. A rota final dá uma
  volta só, mas com um trecho de 9 pontos percorrido ao contrário, ligado ao resto por
  duas cordas: só essas duas cordas explicam os 3,2% que faltam.
- O custo cresce rápido com N. O tempo por época vai de ~16 ms (N = 100) a ~100 ms
  (N = 500), porque cada filho custa `O(N)`. E o número de épocas também cresce, porque
  a inversão certa tem chance da ordem de `1/N²`. É o limite do AG puro de que a
  decisão 006 fala: com mais pontos, o próximo ganho viria de busca local (2-opt
  memético), não de ajuste de parâmetros.

## 9. Como reproduzir

Requisitos: MSYS2 UCRT64 (`g++`, `cmake`, `ninja`; ver o README) e
[uv](https://docs.astral.sh/uv/) para os scripts Python.

```powershell
powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1            # tudo (~25 min com o bônus)
powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1 -SemBonus  # ~5 min
```

O script compila, roda os testes (`ctest`), roda os cenários em `saida/`, gera as
figuras e as copia para `docs/figuras/`. Execuções que já existem são puladas
(`-Refazer` roda de novo). As varreduras de parâmetros (seções 2 e 3) não são refeitas,
porque levam horas: os CSVs estão em `docs/experimentos/`, e os comandos, no README.
Figuras de uma pasta só: `uv run viz/plotar.py saida/<pasta> --gif`.

A mesma seed só reproduz os mesmos números com o mesmo compilador (g++ do MSYS2). Os
tempos dependem da máquina.

## Lista de conferência

- [x] **População e parada** (Pessoa C): varredura de população; gráfico do limite de estagnação; curvas mostrando onde a melhora para.
- [x] **Mutação** (Pessoa C): mapas de calor população × mutação; curvas e diversidade com mutação 0, 0,5 e 1.
- [x] **Gene e cruzamento** (Pessoa B): explicação do ERX; a comparação ERX × OX não foi feita (só o ERX foi implementado).
- [x] **Aptidão** (Pessoa A): explicação de `1/distância`.
- [x] **Aleatório** (Pessoa D): curvas por época; rotas por época (grade e GIF); força bruta (N = 10), vizinho mais próximo e 2-opt (N = 50); média ± desvio em 10 seeds.
- [x] **Circular** (Pessoa D): o mesmo, mais o gap para o ótimo.
- [x] **Bônus** (Pessoa D): círculo com N = 100, 200 e 500; tempo até gap ≤ 5%, ≤ 1% e até a última melhora; rotas intermediárias.
