"""Distâncias de referência que o `tsp` não calcula, a partir do pontos.csv.

    vizinho_mais_proximo  heurística gulosa, a melhor entre todas as cidades de partida
    forca_bruta           ótimo exato (Held-Karp), só com N <= 10
    referencia_2opt       uniforme com N > 10: 2-opt + Or-opt com 300 reinícios, a mesma
                          busca e a mesma seed de scripts/referencia_2opt.py (Pessoa C)

O ótimo do círculo já vem no resumo.txt (chave `otimo`). Os valores ficam em cache
em <pasta pai>/referencias.json, indexados pelo conteúdo do pontos.csv: as 10 seeds
de um cenário usam os mesmos pontos e calculam uma vez só.
"""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import numpy as np

RAIZ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / "scripts"))
import referencia_2opt  # noqa: E402  (scripts/referencia_2opt.py, da Pessoa C)

LIMITE_FORCA_BRUTA = 10
REINICIOS_2OPT = 300  # o mesmo padrão de scripts/referencia_2opt.py


def matriz(coords: np.ndarray) -> np.ndarray:
    return np.sqrt(((coords[:, None, :] - coords[None, :, :]) ** 2).sum(-1))


def vizinho_mais_proximo(dist: np.ndarray) -> float:
    n = len(dist)
    melhor = np.inf
    for inicio in range(n):
        visitado = np.zeros(n, dtype=bool)
        visitado[inicio] = True
        atual, total = inicio, 0.0
        for _ in range(n - 1):
            candidatos = np.where(visitado, np.inf, dist[atual])
            prox = int(candidatos.argmin())
            total += candidatos[prox]
            visitado[prox] = True
            atual = prox
        melhor = min(melhor, total + dist[atual, inicio])
    return float(melhor)


def forca_bruta(dist: np.ndarray) -> float:
    """Ótimo exato por programação dinâmica (Held-Karp), partindo da cidade 0."""
    n = len(dist)
    custo = {(1 << j, j): dist[0, j] for j in range(1, n)}
    for _ in range(2, n):  # cada passo acrescenta uma cidade ao caminho
        novo = {}
        for (conjunto, ultimo), c in custo.items():
            for j in range(1, n):
                bit = 1 << j
                if conjunto & bit:
                    continue
                chave = (conjunto | bit, j)
                valor = c + dist[ultimo, j]
                if valor < novo.get(chave, np.inf):
                    novo[chave] = valor
        custo = novo
    return float(min(c + dist[ultimo, 0] for (_, ultimo), c in custo.items()))


def busca_2opt(dist: np.ndarray, reinicios: int = REINICIOS_2OPT) -> float:
    """Mesmo laço do main() de scripts/referencia_2opt.py."""
    rng = np.random.default_rng(0)
    melhor = np.inf
    for _ in range(reinicios):
        rota = rng.permutation(len(dist))
        while True:
            antes = referencia_2opt.comprimento(rota, dist)
            rota = referencia_2opt.or_opt(referencia_2opt.dois_opt(rota, dist), dist)
            if referencia_2opt.comprimento(rota, dist) >= antes - 1e-10:
                break
        melhor = min(melhor, referencia_2opt.comprimento(rota, dist))
    return float(melhor)


def calcular(coords: np.ndarray, cenario: str) -> dict[str, float]:
    dist = matriz(coords)
    n = len(coords)
    refs = {"vizinho_mais_proximo": vizinho_mais_proximo(dist)}
    if n <= LIMITE_FORCA_BRUTA:
        refs["forca_bruta"] = forca_bruta(dist)
    elif cenario == "uniforme":
        print(f"    calculando a referência 2-opt ({n} pontos, {REINICIOS_2OPT} reinícios)...")
        refs["referencia_2opt"] = busca_2opt(dist)
    return refs


def obter(pasta: Path, coords: np.ndarray, cenario: str) -> dict[str, float]:
    chave = hashlib.sha1((pasta / "pontos.csv").read_bytes()).hexdigest()
    cache_caminho = pasta.parent / "referencias.json"
    try:
        cache = json.loads(cache_caminho.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError):
        cache = {}
    if chave not in cache:
        cache[chave] = calcular(coords, cenario)
        cache_caminho.write_text(json.dumps(cache, indent=1), encoding="utf-8")
    return cache[chave]
