# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy>=1.26"]
# ///
"""Gera dados de EXEMPLO no formato de saída de docs/divisao-trabalho.md.

Os números são falsos. As execuções vêm de um AG mínimo em Python
(torneio + inversão, sem cruzamento) e o experimentos.csv é sintético.
Servem só para desenvolver os gráficos antes de o executável C++ existir.
Não usar no relatório.

Uso (na raiz do repo):
    uv run viz/gerar_exemplo.py [--saida saida_exemplo]
"""

from __future__ import annotations

import argparse
import itertools
import math
import time
from pathlib import Path

import numpy as np

RAIZ = Path(__file__).resolve().parent.parent
RAIO = 1.0  # proposta: círculo de raio 1, uniforme em [0, 1]²
ORIGEM = "exemplo (dados falsos de viz/gerar_exemplo.py)"

# Parâmetros do AG mínimo. Só existem para gerar curvas com cara de AG.
PARAMETROS = {
    "populacao": 100,
    "taxa_cruzamento": 0.0,
    "taxa_mutacao": 0.9,
    "torneio": 3,
    "elite": 2,
    "estagnacao": 400,
    "cruzamento": "nenhum",
}

# (cenario, n, seed, max_epocas, intervalo_rotas)
# O "bônus" de exemplo usa N = 200 para chegar ao ótimo em ~10 s.
EXECUCOES = [
    ("uniforme", 10, 1, 300, 1),
    ("uniforme", 40, 1, 2000, 1),
    ("circulo", 10, 1, 300, 1),
    ("circulo", 40, 1, 2000, 1),
    ("circulo", 200, 1, 4000, 10),
]


def candidatos_inversao(n: int) -> int:
    """Quantas inversões a mutação testa antes de aplicar a melhor."""
    return max(4, n // 25)


# ---------------------------------------------------------------- cenários

def gerar_pontos(cenario: str, n: int, rng: np.random.Generator) -> np.ndarray:
    if cenario == "uniforme":
        return rng.random((n, 2))
    angulos = 2 * np.pi * np.arange(n) / n
    pontos = np.column_stack([RAIO * np.cos(angulos), RAIO * np.sin(angulos)])
    return pontos[rng.permutation(n)]  # embaralha os índices


def matriz_distancias(pontos: np.ndarray) -> np.ndarray:
    dif = pontos[:, None, :] - pontos[None, :, :]
    return np.sqrt((dif**2).sum(axis=-1))


def otimo_circulo(n: int) -> float:
    return 2 * n * RAIO * math.sin(math.pi / n)


def vizinho_mais_proximo(dist: np.ndarray, inicio: int = 0) -> float:
    n = len(dist)
    visitado = np.zeros(n, dtype=bool)
    visitado[inicio] = True
    atual, total = inicio, 0.0
    for _ in range(n - 1):
        candidatos = np.where(visitado, np.inf, dist[atual])
        prox = int(candidatos.argmin())
        total += candidatos[prox]
        visitado[prox] = True
        atual = prox
    return total + dist[atual, inicio]


def forca_bruta(dist: np.ndarray) -> float:
    """Ótimo exato fixando a cidade 0 no início. Só para n <= 10."""
    n = len(dist)
    total = math.factorial(n - 1)
    perms = np.fromiter(
        itertools.chain.from_iterable(itertools.permutations(range(1, n))),
        dtype=np.int16,
        count=total * (n - 1),
    ).reshape(total, n - 1)
    comp = (
        dist[0, perms[:, 0]]
        + dist[perms[:, :-1], perms[:, 1:]].sum(axis=1)
        + dist[perms[:, -1], 0]
    )
    return float(comp.min())


# ---------------------------------------------------------------- AG mínimo

def comprimentos(pop: np.ndarray, dist: np.ndarray) -> np.ndarray:
    return dist[pop, np.roll(pop, -1, axis=1)].sum(axis=1)


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


def rodar_ag(dist, otimo, rng, max_epocas, intervalo_rotas):
    p = PARAMETROS
    n, tam = len(dist), p["populacao"]
    inicio = time.perf_counter()
    pop = np.array([rng.permutation(n) for _ in range(tam)])
    comp = comprimentos(pop, dist)

    historico, rotas = [], []
    melhor_global, ultima_melhora = math.inf, 0
    motivo = "max_epocas"

    for epoca in range(max_epocas + 1):
        if epoca > 0:
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
        historico.append(
            (epoca, comp[b], comp.mean(), comp.max(), time.perf_counter() - inicio)
        )
        if comp[b] < melhor_global - 1e-12:
            melhor_global, ultima_melhora = comp[b], epoca

        parar = None
        if otimo is not None and comp[b] <= otimo * (1 + 1e-9):
            parar = "meta"
        elif epoca - ultima_melhora >= p["estagnacao"]:
            parar = "estagnacao"
        elif epoca == max_epocas:
            parar = "max_epocas"

        # rotas.csv: a cada `intervalo_rotas` épocas e sempre a última
        if epoca % intervalo_rotas == 0 or parar:
            rotas.append((epoca, comp[b], pop[b].copy()))
        if parar:
            motivo = parar
            break

    return historico, rotas, motivo, ultima_melhora


# ---------------------------------------------------------------- escrita

def gravar_execucao(pasta: Path, cenario, n, seed, max_epocas, intervalo_rotas):
    rng = np.random.default_rng(seed)
    pontos = gerar_pontos(cenario, n, rng)
    dist = matriz_distancias(pontos)

    otimo = otimo_circulo(n) if cenario == "circulo" else None
    bruta = forca_bruta(dist) if cenario == "uniforme" and n <= 10 else None
    alvo = otimo if otimo is not None else bruta
    vizinho = vizinho_mais_proximo(dist)

    historico, rotas, motivo, ultima_melhora = rodar_ag(
        dist, alvo, rng, max_epocas, intervalo_rotas
    )

    pasta.mkdir(parents=True, exist_ok=True)
    with open(pasta / "pontos.csv", "w", newline="") as f:
        f.write("indice,x,y\n")
        for i, (x, y) in enumerate(pontos):
            f.write(f"{i},{x:.6f},{y:.6f}\n")

    with open(pasta / "historico.csv", "w", newline="") as f:
        f.write("epoca,melhor,media,pior,tempo_s\n")
        for e, m, med, pior, t in historico:
            f.write(f"{e},{m:.6f},{med:.6f},{pior:.6f},{t:.6f}\n")

    with open(pasta / "rotas.csv", "w", newline="") as f:
        f.write("epoca,distancia,rota\n")
        for e, d, rota in rotas:
            f.write(f"{e},{d:.6f},{' '.join(map(str, rota))}\n")

    melhor = historico[-1][1]
    resumo = {
        "origem": ORIGEM,
        "cenario": cenario,
        "n": n,
        "seed": seed,
        **PARAMETROS,
        "max_epocas": max_epocas,
        "intervalo_rotas": intervalo_rotas,
        "epocas": historico[-1][0],
        "ultima_melhora": ultima_melhora,
        "melhor": f"{melhor:.6f}",
        "vizinho_mais_proximo": f"{vizinho:.6f}",
    }
    if cenario == "circulo":
        resumo["raio"] = RAIO
        resumo["otimo"] = f"{otimo:.6f}"
    if bruta is not None:
        resumo["forca_bruta"] = f"{bruta:.6f}"
    if alvo is not None:
        resumo["gap_pct"] = f"{(melhor / alvo - 1) * 100:.4f}"
    resumo["motivo_parada"] = motivo
    resumo["tempo_total_s"] = f"{historico[-1][4]:.6f}"

    with open(pasta / "resumo.txt", "w", newline="") as f:
        for chave, valor in resumo.items():
            f.write(f"{chave}={valor}\n")

    gap = resumo.get("gap_pct", "-")
    print(
        f"  {pasta.name}: {historico[-1][0]} épocas, melhor={melhor:.4f}, "
        f"gap={gap}%, parada={motivo}, {historico[-1][4]:.1f}s"
    )


def gravar_experimentos(caminho: Path, rng: np.random.Generator) -> None:
    """experimentos.csv sintético: gap = efeito da população + da mutação + ruído."""
    populacoes = [50, 100, 200, 400]
    mutacoes = [0.02, 0.05, 0.1, 0.2, 0.4]
    mutacao_ideal = {"erx": 0.2, "ox": 0.1}  # ERX perde diversidade mais rápido
    fator_cruz = {"erx": 0.8, "ox": 1.0}

    linhas = []
    for cenario, n in [("uniforme", 40), ("circulo", 40)]:
        if cenario == "circulo":
            referencia = otimo_circulo(n)
            base = referencia  # gap medido contra o ótimo
        else:
            pontos = gerar_pontos("uniforme", n, np.random.default_rng(0))
            referencia = vizinho_mais_proximo(matriz_distancias(pontos))
            base = referencia / 1.2  # o AG costuma bater o vizinho mais próximo
        for pop, mut, cruz in itertools.product(populacoes, mutacoes, mutacao_ideal):
            gap = 8 * (50 / pop) ** 0.6 + 2 * math.log(mut / mutacao_ideal[cruz]) ** 2
            gap *= fator_cruz[cruz]
            for seed in range(1, 11):
                g = gap * rng.lognormal(0, 0.25)
                epocas = max(50, int(rng.normal(900 * (100 / pop) ** 0.3, 120)))
                tempo = epocas * pop * n * 2e-7 * (1.5 if cruz == "erx" else 1.0)
                linhas.append(
                    f"{cenario},{n},{pop},{mut},{cruz},{seed},"
                    f"{base * (1 + g / 100):.6f},{epocas},{tempo:.6f},{referencia:.6f}"
                )

    with open(caminho, "w", newline="") as f:
        f.write(
            "cenario,n,populacao,taxa_mutacao,cruzamento,seed,"
            "melhor,epocas,tempo_s,referencia\n"
        )
        f.write("\n".join(linhas) + "\n")
    print(f"  {caminho.name}: {len(linhas)} linhas")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--saida",
        type=Path,
        default=RAIZ / "saida_exemplo",
        help="pasta de destino (padrão: saida_exemplo/ na raiz do repo)",
    )
    args = parser.parse_args()

    print(f"Gerando dados de EXEMPLO em {args.saida}")
    for cenario, n, seed, max_epocas, intervalo in EXECUCOES:
        pasta = args.saida / f"{cenario}_n{n}_s{seed}"
        gravar_execucao(pasta, cenario, n, seed, max_epocas, intervalo)
    gravar_experimentos(args.saida / "experimentos.csv", np.random.default_rng(42))


if __name__ == "__main__":
    main()
