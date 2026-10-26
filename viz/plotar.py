# /// script
# requires-python = ">=3.11"
# dependencies = ["matplotlib>=3.8", "numpy>=1.26", "pandas>=2.1", "pillow>=10"]
# ///
"""Figuras de uma execução do AG: curva por época, grade de rotas, GIF e gap.

Uso (na raiz do repo):
    uv run viz/plotar.py saida/circulo_n40_s1 [outras pastas...] [--gif] [--n-rotas 9]

Lê pontos.csv, historico.csv, rotas.csv e resumo.txt (formato em
docs/divisao-trabalho.md) e grava as figuras em <pasta>/figuras/:
    curva.png        melhor/média/pior por época, com as referências
    rotas_grade.png  melhor rota em algumas épocas
    evolucao.gif     animação da melhor rota (com --gif)
    gap.png          gap para o ótimo por época e por tempo (se houver ótimo)
    marcos.txt       tempo até gap <= 5%, <= 1% e até a última melhora
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

import estilo  # liga o backend Agg antes do pyplot
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from estilo import AGUA, AZUL, LARANJA, TINTA_2, TINTA_FRACA
from matplotlib.animation import FuncAnimation, PillowWriter

PISO_GAP = 1e-3  # gap 0 vira 0,001% no eixo log


# ---------------------------------------------------------------- leitura

def ler_resumo(pasta: Path) -> dict[str, str]:
    caminho = pasta / "resumo.txt"
    if not caminho.exists():
        return {}
    resumo = {}
    for linha in caminho.read_text(encoding="utf-8").splitlines():
        if "=" in linha and not linha.lstrip().startswith("#"):
            chave, valor = linha.split("=", 1)
            resumo[chave.strip()] = valor.strip()
    return resumo


def numero(resumo: dict[str, str], *chaves: str) -> float | None:
    """Primeiro valor numérico entre as chaves; None se nenhuma existir."""
    for chave in chaves:
        try:
            return float(resumo[chave])
        except (KeyError, ValueError):
            continue
    return None


def ler_rotas(pasta: Path) -> list[tuple[int, float, np.ndarray]]:
    rotas = []
    with open(pasta / "rotas.csv", encoding="utf-8") as f:
        next(f)  # cabeçalho
        for linha in f:
            linha = linha.strip()
            if linha:
                epoca, dist, rota = linha.split(",", 2)
                rotas.append((int(epoca), float(dist), np.array(rota.split(), dtype=int)))
    return rotas


def ler_execucao(pasta: Path):
    faltando = [
        nome for nome in ("pontos.csv", "historico.csv", "rotas.csv") if not (pasta / nome).exists()
    ]
    if faltando:
        raise SystemExit(f"Erro: faltando {', '.join(faltando)} em {pasta}")
    pontos = pd.read_csv(pasta / "pontos.csv").sort_values("indice")
    coords = pontos[["x", "y"]].to_numpy()
    historico = pd.read_csv(pasta / "historico.csv")
    return coords, historico, ler_rotas(pasta), ler_resumo(pasta)


# ---------------------------------------------------------------- apoio

def eh_exemplo(resumo) -> bool:
    return resumo.get("origem", "").startswith("exemplo")


def alvo(resumo) -> float | None:
    """Ótimo conhecido: fórmula do círculo ou força bruta (uniforme, N <= 10)."""
    return numero(resumo, "otimo", "forca_bruta")


def referencias(resumo) -> list[tuple[str, float]]:
    refs = []
    for chave, nome in [
        ("otimo", "ótimo"),
        ("forca_bruta", "força bruta (ótimo)"),
        ("vizinho_mais_proximo", "vizinho mais próximo"),
    ]:
        valor = numero(resumo, chave)
        if valor is not None:
            refs.append((nome, valor))
    return refs


def epoca_ultima_melhora(historico: pd.DataFrame) -> int:
    melhorou = historico["melhor"].diff() < -1e-12
    return int(historico["epoca"][melhorou].max()) if melhorou.any() else 0


def escolher(itens: list, k: int) -> list:
    """Até k itens, mais densos no começo (é onde a rota mais muda)."""
    if len(itens) <= k:
        return list(itens)
    idx = set(np.round(np.linspace(0, 1, k) ** 2 * (len(itens) - 1)).astype(int))
    for extra in np.round(np.linspace(0, len(itens) - 1, k)).astype(int):
        if len(idx) >= k:
            break
        idx.add(int(extra))
    return [itens[i] for i in sorted(idx)]


def tamanho_marcas(n: int) -> tuple[float, float]:
    """(área do ponto, espessura da rota) conforme o número de cidades."""
    if n <= 50:
        return 18, 1.6
    if n <= 300:
        return 5, 1.0
    return 1.5, 0.6


def desenhar_rota(ax, coords, rota, lw):
    fechada = np.append(rota, rota[0])
    return ax.plot(coords[fechada, 0], coords[fechada, 1], color=AZUL, lw=lw, zorder=2)[0]


def preparar_mapa(ax, coords, s):
    ax.scatter(coords[:, 0], coords[:, 1], s=s, color=TINTA_2, zorder=3, linewidths=0)
    ax.set_aspect("equal")
    ax.set_xticks([])
    ax.set_yticks([])
    ax.grid(False)
    for lado in ax.spines.values():
        lado.set_visible(False)
    margem = 0.04 * max(np.ptp(coords[:, 0]), np.ptp(coords[:, 1]), 1e-9)
    ax.set_xlim(coords[:, 0].min() - margem, coords[:, 0].max() + margem)
    ax.set_ylim(coords[:, 1].min() - margem, coords[:, 1].max() + margem)


def texto_rota(epoca, dist, otimo) -> str:
    texto = f"época {epoca} · d = {dist:.4g}"
    if otimo:
        texto += f" · gap {(dist / otimo - 1) * 100:.2f}%"
    return texto


# ---------------------------------------------------------------- figuras

def plotar_curva(historico, resumo, nome, destino: Path) -> None:
    fig, ax = plt.subplots(figsize=(8, 4.6))
    x = historico["epoca"]
    ax.plot(x, historico["pior"], color=AGUA, lw=1.5, label="pior", zorder=2)
    ax.plot(x, historico["media"], color=LARANJA, lw=1.5, label="média", zorder=3)
    ax.plot(x, historico["melhor"], color=AZUL, lw=2, label="melhor", zorder=4)

    if historico["pior"].iloc[0] / historico["melhor"].min() > 5:
        estilo.eixo_y_log(ax)
    estilo.rotular_referencias(ax, referencias(resumo))

    ultima = epoca_ultima_melhora(historico)
    ax.axvline(ultima, color=TINTA_FRACA, lw=1, ls=(0, (1, 2)), zorder=1)
    perto_da_borda = ultima > 0.75 * max(int(x.iloc[-1]), 1)
    ax.annotate(
        f"última melhora\népoca {ultima}",
        xy=(ultima, 1),
        xycoords=("data", "axes fraction"),
        xytext=(-4 if perto_da_borda else 4, -4),
        textcoords="offset points",
        ha="right" if perto_da_borda else "left",
        va="top",
        fontsize=8.5,
        color=TINTA_2,
    )

    melhor = historico["melhor"].iloc[-1]
    partes = [
        f"{int(x.iloc[-1])} épocas",
        f"{historico['tempo_s'].iloc[-1]:.2f} s",
        f"melhor = {melhor:.4g}",
    ]
    if (ot := alvo(resumo)) is not None:
        partes.append(f"gap = {(melhor / ot - 1) * 100:.2f}%")
    if "motivo_parada" in resumo:
        partes.append(f"parada: {resumo['motivo_parada']}")
    estilo.titulo(ax, f"{nome}: distância por época", " · ".join(partes))
    ax.set_xlabel("época")
    ax.set_ylabel("distância da rota")
    ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.14), ncols=3, fontsize=9)
    estilo.marcar_exemplo(fig, eh_exemplo(resumo))
    fig.savefig(destino)
    plt.close(fig)


def plotar_grade(coords, rotas, resumo, nome, destino: Path, k: int) -> None:
    escolhidas = escolher(rotas, k)
    colunas = 3 if len(escolhidas) <= 9 else 4
    linhas = math.ceil(len(escolhidas) / colunas)
    fig, eixos = plt.subplots(
        linhas, colunas, figsize=(3.2 * colunas, 3.4 * linhas + 0.4), squeeze=False
    )
    s, lw = tamanho_marcas(len(coords))
    ot = alvo(resumo)
    for ax, (epoca, dist, rota) in zip(eixos.flat, escolhidas):
        preparar_mapa(ax, coords, s)
        desenhar_rota(ax, coords, rota, lw)
        ax.set_title(texto_rota(epoca, dist, ot), fontsize=9, fontweight="normal", color=TINTA_2)
    for ax in list(eixos.flat)[len(escolhidas) :]:
        ax.axis("off")
    fig.suptitle(f"{nome}: melhor rota ao longo das épocas", x=0.02, ha="left",
                 fontsize=12, fontweight="semibold")
    fig.tight_layout()
    estilo.marcar_exemplo(fig, eh_exemplo(resumo))
    fig.savefig(destino)
    plt.close(fig)


def gerar_gif(coords, rotas, resumo, nome, destino: Path, quadros=80, fps=10) -> None:
    escolhidas = escolher(rotas, quadros)
    escolhidas += [escolhidas[-1]] * (2 * fps)  # segura o quadro final por 2 s
    fig, ax = plt.subplots(figsize=(5.5, 5.9), dpi=90)
    s, lw = tamanho_marcas(len(coords))
    preparar_mapa(ax, coords, s)
    ot = alvo(resumo)
    linha = desenhar_rota(ax, coords, escolhidas[0][2], lw)
    ax.set_title(nome, fontsize=11)
    estilo.marcar_exemplo(fig, eh_exemplo(resumo))
    legenda = ax.annotate("", xy=(0, 0), xycoords="axes fraction", xytext=(0, -14),
                          textcoords="offset points", fontsize=9, color=TINTA_2)

    def atualizar(quadro):
        epoca, dist, rota = quadro
        fechada = np.append(rota, rota[0])
        linha.set_data(coords[fechada, 0], coords[fechada, 1])
        legenda.set_text(texto_rota(epoca, dist, ot))
        return linha, legenda

    animacao = FuncAnimation(fig, atualizar, frames=escolhidas, blit=False)
    animacao.save(destino, writer=PillowWriter(fps=fps), dpi=90)
    plt.close(fig)


def calcular_marcos(historico, ot: float) -> dict[str, str]:
    gap = (historico["melhor"] / ot - 1) * 100
    marcos = {}
    for limite in (5, 1):
        atingiu = historico[gap <= limite + 1e-9]
        if len(atingiu):
            marcos[f"epoca_gap_{limite}pct"] = str(int(atingiu["epoca"].iloc[0]))
            marcos[f"tempo_gap_{limite}pct_s"] = f"{atingiu['tempo_s'].iloc[0]:.4f}"
        else:
            marcos[f"epoca_gap_{limite}pct"] = "nao_atingido"
            marcos[f"tempo_gap_{limite}pct_s"] = "nao_atingido"
    ultima = epoca_ultima_melhora(historico)
    marcos["epoca_ultima_melhora"] = str(ultima)
    tempo_ultima = historico.loc[historico["epoca"] == ultima, "tempo_s"].iloc[0]
    marcos["tempo_ultima_melhora_s"] = f"{tempo_ultima:.4f}"
    marcos["gap_final_pct"] = f"{gap.iloc[-1]:.4f}"
    marcos["epocas_total"] = str(int(historico["epoca"].iloc[-1]))
    marcos["tempo_total_s"] = f"{historico['tempo_s'].iloc[-1]:.4f}"
    return marcos


def plotar_gap(historico, ot, marcos, nome, destino: Path, exemplo: bool) -> None:
    gap = ((historico["melhor"] / ot - 1) * 100).clip(lower=PISO_GAP)
    fig, eixos = plt.subplots(1, 2, figsize=(10, 4.2), sharey=True)
    for ax, coluna, rotulo in [
        (eixos[0], "epoca", "época"),
        (eixos[1], "tempo_s", "tempo (s)"),
    ]:
        ax.plot(historico[coluna], gap, color=AZUL, lw=2)
        estilo.eixo_y_log(ax)
        ax.set_xlabel(rotulo)
        for limite in (5, 1):
            ax.axhline(limite, color=TINTA_FRACA, lw=1, ls=(0, (4, 3)))
            chave = f"epoca_gap_{limite}pct"
            if marcos[chave] != "nao_atingido":
                epoca = int(marcos[chave])
                xval = historico.loc[historico["epoca"] == epoca, coluna].iloc[0]
                ax.plot(xval, limite, "o", ms=7, color=AZUL, mec=estilo.SUPERFICIE, mew=2, zorder=5)
                texto = f"≤ {limite}%: época {epoca}" if coluna == "epoca" else \
                        f"≤ {limite}%: {float(marcos[f'tempo_gap_{limite}pct_s']):.2f} s"
                ax.annotate(texto, xy=(xval, limite), xytext=(6, 4), textcoords="offset points",
                            fontsize=8.5, color=TINTA_2)
    eixos[0].set_ylabel("gap para o ótimo (%, escala log)")
    sub = (
        f"última melhora: época {marcos['epoca_ultima_melhora']} "
        f"({float(marcos['tempo_ultima_melhora_s']):.2f} s) · "
        f"gap final {float(marcos['gap_final_pct']):.2f}%"
    )
    estilo.titulo(eixos[0], f"{nome}: convergência", sub)
    fig.tight_layout()
    estilo.marcar_exemplo(fig, exemplo)
    fig.savefig(destino)
    plt.close(fig)


# ---------------------------------------------------------------- main

def processar(pasta: Path, gif: bool, n_rotas: int) -> None:
    coords, historico, rotas, resumo = ler_execucao(pasta)
    nome = estilo.nome_execucao(
        resumo.get("cenario"), resumo.get("n", len(coords)), resumo.get("seed")
    )
    figuras = pasta / "figuras"
    figuras.mkdir(exist_ok=True)
    feitas = []

    plotar_curva(historico, resumo, nome, figuras / "curva.png")
    feitas.append("curva.png")
    plotar_grade(coords, rotas, resumo, nome, figuras / "rotas_grade.png", n_rotas)
    feitas.append("rotas_grade.png")
    if gif:
        gerar_gif(coords, rotas, resumo, nome, figuras / "evolucao.gif")
        feitas.append("evolucao.gif")

    ot = alvo(resumo)
    if ot is not None:
        marcos = calcular_marcos(historico, ot)
        (figuras / "marcos.txt").write_text(
            "".join(f"{k}={v}\n" for k, v in marcos.items()), encoding="utf-8"
        )
        plotar_gap(historico, ot, marcos, nome, figuras / "gap.png", eh_exemplo(resumo))
        feitas += ["gap.png", "marcos.txt"]

    print(f"  {pasta}: {', '.join(feitas)}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pastas", nargs="+", type=Path, help="pasta(s) de execução")
    parser.add_argument("--gif", action="store_true", help="gera também o evolucao.gif")
    parser.add_argument("--n-rotas", type=int, default=9, help="rotas na grade (padrão 9)")
    args = parser.parse_args()
    for pasta in args.pastas:
        if not pasta.is_dir():
            sys.exit(f"Erro: {pasta} não é uma pasta")
        processar(pasta, args.gif, args.n_rotas)


if __name__ == "__main__":
    main()
