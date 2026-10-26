# Algoritmo genético para o caixeiro viajante — relatório

> **Esqueleto (Pessoa D).** Cada seção diz quem escreve o texto e quais figuras
> entram. As figuras saem de `scripts/rodar_tudo.ps1`, que as copia para
> `docs/figuras/`. Enquanto o C++ não existe, as figuras de exemplo ficam em
> `saida_exemplo/` e não entram aqui. A lista de conferência está no fim.

## 1. Problema e cenários

_Texto: Pessoa A._

- Pontos em 2D, caminho cíclico que passa por todos, distância euclidiana, N arbitrário (≥ 8).
- **Uniforme**: N pontos sorteados em [0, 1]².
- **Círculo** (benchmark): N pontos igualmente espaçados num círculo de raio R, com
  os índices embaralhados. O ótimo é conhecido: `2·N·R·sin(π/N)`.
- Referências: força bruta (N ≤ 10) e vizinho mais próximo.

## 2. População e critério de parada (1 ponto)

_Texto: Pessoa C._

- Tamanho de população escolhido e por quê: _..._
- Critério de parada: o primeiro entre máximo de épocas, estagnação e meta (gap no círculo). Valores: _..._

![Efeito da população: cada ponto é uma seed](figuras/experimentos/populacao_seeds.png)

![Curva do círculo com N = 40; a linha pontilhada marca a última melhora](figuras/circulo_n40_s1/curva.png)

## 3. Taxa de mutação (1 ponto)

_Texto: Pessoa C._

- Mutação = probabilidade, por filho, de aplicar uma inversão. Valor escolhido: _..._
- Por que mais alta que com OX (decisão 002): o ERX perde diversidade mais rápido.

![Mapa de calor população × mutação, círculo](figuras/experimentos/mapa_calor_circulo_n40.png)

![Mapa de calor população × mutação, uniforme](figuras/experimentos/mapa_calor_uniforme_n40.png)

![Mutação baixa](figuras/circulo_n40_s1_mutbaixa/curva.png)

![Mutação alta](figuras/circulo_n40_s1_mutalta/curva.png)

## 4. Representação do gene e cruzamento (2 pontos)

_Texto: Pessoa B (base: `README.md` e decisões 002 e 004)._

- Gene: permutação dos índices das cidades.
- Cruzamento: ERX, priorizando arestas comuns aos dois pais e depois o menor grau. Exemplo passo a passo: _..._
- Comparação com o OX:

![ERX × OX na melhor configuração de cada um](figuras/experimentos/erx_vs_ox.png)

## 5. Função de aptidão (1 ponto)

_Texto: Pessoa A._

- Aptidão `1/distância`. Como o torneio só compara indivíduos, isso equivale a usar `−distância`.

## 6. Desempenho por época — cenário aleatório (2,5 pontos)

_Texto e figuras: Pessoa D._

### N = 10: comparação com a força bruta

![Curva, uniforme N = 10](figuras/uniforme_n10_s1/curva.png)

![Rotas, uniforme N = 10](figuras/uniforme_n10_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/uniforme_n10_s1/evolucao.gif)

### N = 40: comparação com o vizinho mais próximo

![Curva, uniforme N = 40](figuras/uniforme_n40_s1/curva.png)

![Rotas, uniforme N = 40](figuras/uniforme_n40_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/uniforme_n40_s1/evolucao.gif)

### Variação entre seeds

_Colar aqui as linhas do uniforme de `figuras/experimentos/media_desvio.md`._

Comentário: _..._

## 7. Desempenho por época — cenário circular (2,5 pontos)

_Texto e figuras: Pessoa D._

### N = 10

![Curva, círculo N = 10](figuras/circulo_n10_s1/curva.png)

![Rotas, círculo N = 10](figuras/circulo_n10_s1/rotas_grade.png)

### N = 40

![Curva, círculo N = 40](figuras/circulo_n40_s1/curva.png)

![Gap para o ótimo, círculo N = 40](figuras/circulo_n40_s1/gap.png)

![Rotas, círculo N = 40](figuras/circulo_n40_s1/rotas_grade.png)

Animação: [evolucao.gif](figuras/circulo_n40_s1/evolucao.gif)

### Variação entre seeds e gap

_Colar aqui as linhas do círculo de `figuras/experimentos/media_desvio.md`._

Comentário: _..._

## 8. Bônus: círculo com N grande (até 1 ponto)

_Texto e figuras: Pessoa D. Valores de `figuras/circulo_n<N>_s1/marcos.txt`._

| N | Épocas até gap ≤ 5% | Tempo (s) | Épocas até gap ≤ 1% | Tempo (s) | Última melhora (época) | Tempo (s) | Gap final (%) |
|---|---|---|---|---|---|---|---|
| 100 | | | | | | | |
| 500 | | | | | | | |
| 1000 | | | | | | | |

![Convergência, círculo N = 1000](figuras/circulo_n1000_s1/gap.png)

![Rotas intermediárias, círculo N = 1000](figuras/circulo_n1000_s1/rotas_grade.png)

Comentário: _..._

## 9. Como reproduzir

Requisitos: MSYS2 ucrt64 (g++, cmake, ninja) e [uv](https://docs.astral.sh/uv/).

```
powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1
```

A mesma seed só reproduz os números com o mesmo compilador (g++ do MSYS2).

## Lista de conferência

- [ ] **População e parada** (Pessoa C): varredura de população; curvas mostrando onde a melhora para.
- [ ] **Mutação** (Pessoa C): mapa de calor população × mutação; curvas com mutação baixa e alta.
- [ ] **Gene e cruzamento** (Pessoa B): explicação do ERX; comparação ERX × OX.
- [ ] **Aptidão** (Pessoa A): explicação de `1/distância`.
- [ ] **Aleatório** (Pessoa D): curvas por época; rotas por época (grade e GIF); comparação com força bruta (N ≤ 10) e vizinho mais próximo; média ± desvio em ~10 seeds.
- [ ] **Circular** (Pessoa D): o mesmo, mais o gap (%) para o ótimo.
- [ ] **Bônus** (Pessoa D): círculo com N grande; tempo até gap ≤ 5%, ≤ 1% e até a última melhora; rotas intermediárias.
