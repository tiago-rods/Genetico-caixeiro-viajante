# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy>=1.26"]
# ///
"""Gera dados de EXEMPLO no mesmo formato que o `tsp` grava (ver viz/dados.py).

Os números são falsos: as execuções vêm de um AG mínimo em Python (torneio +
inversão, sem cruzamento). Servem só para testar os gráficos sem compilar o C++.
Não usar no relatório.

Uso (na raiz do repo):
    uv run viz/gerar_exemplo.py [--saida saida_exemplo]
"""

from __future__ import annotations

import argparse
import math
import time
from pathlib import Path

import numpy as np

RAIZ = Path(__file__).resolve().parent.parent
LADO = 100.0  # como no main.cpp: uniforme em [0, 100]²
RAIO = 50.0  # e círculo de raio 50 centrado em (50, 50)
ORIGEM = "exemplo (dados falsos de viz/gerar_exemplo.py)"

# Parâmetros do AG mínimo. Só existem para gerar curvas com cara de AG.
PARAMETROS = {
    "populacao": 100,
    "taxa_cruzamento": 0.0,
    "taxa_mutacao": 0.9,
    "torneio": 3,
    "elite": 2,
    "estagnacao": 400,
}

# (cenario, n, seed, max_geracoes). O "bônus" de exemplo usa N = 200.
EXECUCOES = [
    ("uniforme", 10, 1, 300),
    ("uniforme", 50, 1, 2000),
    ("circulo", 10, 1, 300),
    ("circulo", 50, 1, 2000),
    ("circulo", 200, 1, 4000),
]


def candidatos_inversao(n: int) -> int:
    """Quantas inversões a mutação testa antes de aplicar a melhor."""
    return max(4, n // 25)


# ---------------------------------------------------------------- cenários

def gerar_pontos(cenario: str, n: int, rng: np.random.Generator) -> np.ndarray:
    if cenario == "uniforme":
        return LADO * rng.random((n, 2))
    angulos = 2 * np.pi * np.arange(n) / n
    return np.column_stack([50 + RAIO * np.cos(angulos), 50 + RAIO * np.sin(angulos)])


def matriz_distancias(pontos: np.ndarray) -> np.ndarray:
    dif = pontos[:, None, :] - pontos[None, :, :]
    return np.sqrt((dif**2).sum(axis=-1))


def otimo_circulo(n: int) -> float:
    return 2 * n * RAIO * math.sin(math.pi / n)


# ---------------------------------------------------------------- AG mínimo

def comprimentos(pop: np.ndarray, dist: np.ndarray) -> np.ndarray:
    return dist[pop, np.roll(pop, -1, axis=1)].sum(axis=1)


def diversidade(pop: np.ndarray) -> float:
    """Arestas distintas na população / n, como no AlgoritmoGenetico.cpp."""
    n = pop.shape[1]
    a, b = pop, np.roll(pop, -1, axis=1)
    return len(np.unique(np.minimum(a, b) * n + np.maximum(a, b))) / n


def inverter(rota: np.ndarray, dist: np.ndarray, rng: np.random.Generator) -> None:
    """Testa algumas inversões aleatórias e aplica a de menor custo (no lugar)."""
    n = len(rota)
    i = rng.integers(1, n - 1, size=candidatos_inversao(n))
    j = rng.integers(i + 1, n)
    a, b = rota[i - 1], rota[i]
    c, d = rota[j], rota[(j + 1) % n]
    delta = dist[a, c] + dist[b, d] - dist[a, b] - dist[c, d]
    k = int(delta.argmin())
    rota[i[k] : j[k] + 1] = rota[i[k] : j[k] + 1][::-1].copy()


def rodar_ag(dist, otimo, rng, max_geracoes):
    p = PARAMETROS
    n, tam = len(dist), p["populacao"]
    inicio = time.perf_counter()
    pop = np.array([rng.permutation(n) for _ in range(tam)])
    comp = comprimentos(pop, dist)

    historico, melhorias = [], []
    melhor_global, ultima_melhora = math.inf, 0
    motivo = "limite_geracoes"

    for geracao in range(max_geracoes + 1):
        if geracao > 0:
            ordem = np.argsort(comp)
            nova = np.empty_like(pop)
            nova[: p["elite"]] = pop[ordem[: p["elite"]]]
            cand = rng.integers(0, tam, size=(tam - p["elite"], p["torneio"]))
            vencedores = cand[np.arange(len(cand)), comp[cand].argmin(axis=1)]
            filhos = pop[vencedores].copy()
            for k in np.flatnonzero(rng.random(len(filhos)) < p["taxa_mutacao"]):
                inverter(filhos[k], dist, rng)
            nova[p["elite"] :] = filhos
            pop, comp = nova, comprimentos(nova, dist)

        b = int(comp.argmin())
        t_ms = (time.perf_counter() - inicio) * 1000
        historico.append((geracao, comp[b], comp.mean(), comp.max(), comp.std(), diversidade(pop), t_ms))
        if comp[b] < melhor_global * (1 - 1e-9):  # rotas.csv: só quando melhora, como no C++
            melhor_global, ultima_melhora = comp[b], geracao
            melhorias.append((geracao, comp[b], t_ms, pop[b].copy()))

        if otimo is not None and comp[b] <= otimo * (1 + 1e-9):
            break  # atalho do exemplo para terminar rápido; o C++ não para no ótimo
        if geracao - ultima_melhora >= p["estagnacao"]:
            motivo = "estagnacao"
            break

    return historico, melhorias, motivo, ultima_melhora


# ---------------------------------------------------------------- escrita

def gravar_execucao(pasta: Path, cenario, n, seed, max_geracoes):
    rng = np.random.default_rng(seed)
    pontos = gerar_pontos(cenario, n, rng)
    dist = matriz_distancias(pontos)
    otimo = otimo_circulo(n) if cenario == "circulo" else None

    historico, melhorias, motivo, ultima_melhora = rodar_ag(dist, otimo, rng, max_geracoes)

    pasta.mkdir(parents=True, exist_ok=True)
    with open(pasta / "pontos.csv", "w", newline="") as f:
        f.write("indice,x,y\n")
        for i, (x, y) in enumerate(pontos):
            f.write(f"{i},{x:.6f},{y:.6f}\n")

    with open(pasta / "historico.csv", "w", newline="") as f:
        f.write("geracao,melhor,media,pior,desvio,diversidade,tempo_ms\n")
        for g, *valores in historico:
            f.write(f"{g}," + ",".join(f"{v:.6f}" for v in valores) + "\n")

    with open(pasta / "rotas.csv", "w", newline="") as f:
        f.write("geracao,distancia,tempo_ms,rota\n")
        for g, d, t, rota in melhorias:
            f.write(f"{g},{d:.6f},{t:.4f},{' '.join(map(str, rota))}\n")

    melhor = historico[-1][1]
    tempo_ultima = next(m[2] for m in melhorias if m[0] == ultima_melhora)
    resumo = {
        "origem": ORIGEM,
        "cenario": cenario,
        "pontos": n,
        "seed": seed,
        "populacao": PARAMETROS["populacao"],
        "taxa_cruzamento": PARAMETROS["taxa_cruzamento"],
        "taxa_mutacao": PARAMETROS["taxa_mutacao"],
        "torneio": PARAMETROS["torneio"],
        "elite": PARAMETROS["elite"],
        "max_geracoes": max_geracoes,
        "max_estagnacao": PARAMETROS["estagnacao"],
        "melhor_distancia": f"{melhor:.6f}",
    }
    if otimo is not None:
        resumo["otimo"] = f"{otimo:.6f}"
        resumo["gap_percentual"] = f"{(melhor / otimo - 1) * 100:.6f}"
    resumo.update({
        "geracoes": historico[-1][0],
        "motivo_parada": motivo,
        "geracao_ultima_melhora": ultima_melhora,
        "tempo_total_s": f"{historico[-1][6] / 1000:.6f}",
        "tempo_ate_ultima_melhora_s": f"{tempo_ultima / 1000:.6f}",
        "melhor_rota": " ".join(map(str, melhorias[-1][3])),
    })
    with open(pasta / "resumo.txt", "w", newline="") as f:
        for chave, valor in resumo.items():
            f.write(f"{chave}: {valor}\n")

    print(f"  {pasta.name}: {historico[-1][0]} épocas, melhor={melhor:.2f}, parada={motivo}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--saida", type=Path, default=RAIZ / "saida_exemplo",
                        help="pasta de destino (padrão: saida_exemplo/ na raiz do repo)")
    args = parser.parse_args()

    print(f"Gerando dados de EXEMPLO em {args.saida}")
    for cenario, n, seed, max_geracoes in EXECUCOES:
        gravar_execucao(args.saida / f"{cenario}_n{n}_s{seed}", cenario, n, seed, max_geracoes)


if __name__ == "__main__":
    main()
