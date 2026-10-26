"""Distância de referência para o cenário uniforme, que não tem ótimo conhecido.

Roda 2-opt + Or-opt (busca local clássica, independente do AG) a partir de várias
permutações aleatórias e devolve a melhor distância encontrada. Não é o ótimo
garantido, mas é um limite superior apertado: no círculo ela acha o ótimo exato,
e serve pra calcular o gap do AG no uniforme.

uso: python scripts/referencia_2opt.py saidas/uniforme/pontos.csv [reinicios=300]
dependência: numpy
"""
import sys

import numpy as np


def comprimento(rota, dist):
    return dist[rota, np.roll(rota, -1)].sum()


def dois_opt(rota, dist):
    """Inverte o trecho entre duas arestas sempre que isso encurta o ciclo, até não melhorar mais."""
    n = len(rota)
    melhorou = True
    while melhorou:
        melhorou = False
        for i in range(n - 2):
            a, b = rota[i], rota[i + 1]
            c = rota[i + 2:]
            d = np.roll(rota, -1)[i + 2:]
            ganho = dist[a, c] + dist[b, d] - dist[a, b] - dist[c, d]
            if i == 0:
                ganho = ganho[:-1]  # a aresta (n-1 -> 0) é vizinha da (0 -> 1)
            if len(ganho) and ganho.min() < -1e-10:
                j = i + 2 + int(ganho.argmin())
                rota[i + 1:j + 1] = rota[i + 1:j + 1][::-1]
                melhorou = True
    return rota


def or_opt(rota, dist):
    """Move um trecho de 1 a 3 cidades pra outra posição (nos dois sentidos) quando isso encurta o ciclo."""
    n = len(rota)
    melhorou = True
    while melhorou:
        melhorou = False
        for tamanho in (1, 2, 3):
            for i in range(n):
                girada = np.roll(rota, -i)
                trecho, resto = girada[:tamanho], girada[tamanho:]
                ganho_remover = (dist[resto[-1], trecho[0]] + dist[trecho[-1], resto[0]]
                                 - dist[resto[-1], resto[0]])
                a, b = resto[:-1], resto[1:]
                custo_direto = dist[a, trecho[0]] + dist[trecho[-1], b] - dist[a, b]
                custo_invertido = dist[a, trecho[-1]] + dist[trecho[0], b] - dist[a, b]
                k1, k2 = custo_direto.argmin(), custo_invertido.argmin()
                if ganho_remover - min(custo_direto[k1], custo_invertido[k2]) > 1e-10:
                    k, inserido = (k1, trecho) if custo_direto[k1] <= custo_invertido[k2] else (k2, trecho[::-1])
                    rota = np.concatenate([resto[:k + 1], inserido, resto[k + 1:]])
                    melhorou = True
                    break
            if melhorou:
                break
    return rota


def main():
    pontos = np.loadtxt(sys.argv[1], delimiter=",", skiprows=1)[:, 1:3]
    reinicios = int(sys.argv[2]) if len(sys.argv) > 2 else 300
    dist = np.sqrt(((pontos[:, None, :] - pontos[None, :, :]) ** 2).sum(-1))
    rng = np.random.default_rng(0)

    melhor = np.inf
    for _ in range(reinicios):
        rota = rng.permutation(len(pontos))
        while True:  # alterna as duas buscas até nenhuma das duas melhorar
            antes = comprimento(rota, dist)
            rota = or_opt(dois_opt(rota, dist), dist)
            if comprimento(rota, dist) >= antes - 1e-10:
                break
        melhor = min(melhor, comprimento(rota, dist))

    print(f"pontos={len(pontos)} reinicios={reinicios} referencia={melhor:.6f}")


if __name__ == "__main__":
    main()
