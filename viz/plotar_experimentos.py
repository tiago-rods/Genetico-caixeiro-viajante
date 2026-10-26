# /// script
# requires-python = ">=3.11"
# dependencies = ["matplotlib>=3.8", "numpy>=1.26", "pandas>=2.1"]
# ///
"""Figuras das varreduras do AG (experimentos.csv).

Uso (na raiz do repo):
    uv run viz/plotar_experimentos.py saida/experimentos.csv [--saida PASTA]

Grava em <pasta do csv>/figuras/ (ou em --saida):
    mapa_calor_<cenario>_n<N>.png  média das seeds, população × mutação, um painel por cruzamento
    erx_vs_ox.png                  seeds de cada cruzamento na melhor configuração dele
    populacao_seeds.png            seeds por tamanho de população (melhor cruzamento e mutação)
    media_desvio.md                tabela média ± desvio, pronta para o relatório

Com a coluna `referencia` (proposta em docs/divisao-trabalho.md), as figuras
usam o gap para ela, em %. Sem ela, usam a melhor distância.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import estilo  # liga o backend Agg antes do pyplot
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from estilo import AGUA, COR_CRUZAMENTO, SEQUENCIAL, SUPERFICIE, TINTA, TINTA_FRACA

COLUNAS = [
    "cenario", "n", "populacao", "taxa_mutacao", "cruzamento",
    "seed", "melhor", "epocas", "tempo_s",
]


# ---------------------------------------------------------------- dados

def carregar(caminho: Path) -> tuple[pd.DataFrame, bool]:
    if not caminho.exists():
        raise SystemExit(f"Erro: {caminho} não existe")
    df = pd.read_csv(caminho)
    falta = [c for c in COLUNAS if c not in df.columns]
    if falta:
        raise SystemExit(f"Erro: faltam as colunas {', '.join(falta)} em {caminho}")
    df["cruzamento"] = df["cruzamento"].str.lower()
    tem_ref = "referencia" in df.columns and df["referencia"].notna().all()
    df["metrica"] = (df["melhor"] / df["referencia"] - 1) * 100 if tem_ref else df["melhor"]
    return df, tem_ref


def rotulo_metrica(cenario: str, tem_ref: bool) -> str:
    if not tem_ref:
        return "melhor distância"
    if cenario == "circulo":
        return "gap para o ótimo (%)"
    return "diferença para o vizinho mais próximo (%)"


def casas_decimais(vmin: float, vmax: float, tem_ref: bool) -> int:
    """Gap com 1 casa; distância com 3 algarismos significativos, iguais na figura toda."""
    if tem_ref:
        return 1
    maior = max(abs(vmin), abs(vmax), 1e-12)
    return max(0, 2 - int(np.floor(np.log10(maior))))


def ordenar_cruzamentos(cruzamentos) -> list[str]:
    presentes = set(cruzamentos)
    conhecidos = [c for c in COR_CRUZAMENTO if c in presentes]
    return conhecidos + sorted(presentes - set(conhecidos))


def cor(cruzamento: str) -> str:
    return COR_CRUZAMENTO.get(cruzamento, AGUA)


def melhor_config(sub: pd.DataFrame, chaves: list[str]) -> tuple:
    """Configuração com a menor média da métrica sobre as seeds."""
    return sub.groupby(chaves)["metrica"].mean().idxmin()


def marcar_exemplo(fig, caminho: Path) -> None:
    estilo.marcar_exemplo(fig, "exemplo" in str(caminho).lower())


def caixas(ax, dados, cores, posicoes) -> None:
    """Boxplot discreto com as seeds por cima (cada ponto é uma seed)."""
    caixa = ax.boxplot(
        dados, positions=posicoes, widths=0.5, patch_artist=True, showfliers=False,
        medianprops={"color": TINTA, "lw": 1.5},
        whiskerprops={"color": TINTA_FRACA, "lw": 1},
        capprops={"color": TINTA_FRACA, "lw": 1},
        boxprops={"lw": 0},
    )
    for patch, c in zip(caixa["boxes"], cores):
        patch.set_facecolor(c)
        patch.set_alpha(0.22)
    rng = np.random.default_rng(0)
    for x, valores, c in zip(posicoes, dados, cores):
        jitter = x + rng.uniform(-0.12, 0.12, len(valores))
        ax.scatter(jitter, valores, s=24, color=c, edgecolors=SUPERFICIE, linewidths=1, zorder=3)
    ax.grid(axis="x", visible=False)


# ---------------------------------------------------------------- figuras

def mapa_calor(df, cenario, n, tem_ref, destino: Path, origem: Path) -> None:
    sub = df[(df["cenario"] == cenario) & (df["n"] == n)]
    cruzamentos = ordenar_cruzamentos(sub["cruzamento"].unique())
    tabelas = {
        c: sub[sub["cruzamento"] == c].pivot_table(
            index="populacao", columns="taxa_mutacao", values="metrica", aggfunc="mean"
        )
        for c in cruzamentos
    }
    vmin = min(np.nanmin(t.to_numpy()) for t in tabelas.values())
    vmax = max(np.nanmax(t.to_numpy()) for t in tabelas.values())
    seeds = sub.groupby(["cruzamento", "populacao", "taxa_mutacao"]).size().max()
    casas = casas_decimais(vmin, vmax, tem_ref)

    fig, eixos = plt.subplots(
        1, len(cruzamentos), figsize=(4.3 * len(cruzamentos) + 1.3, 4.0),
        squeeze=False, constrained_layout=True,
    )
    imagem = None
    for k, (ax, c) in enumerate(zip(eixos[0], cruzamentos)):
        tabela = tabelas[c]
        valores = tabela.to_numpy()
        imagem = ax.imshow(valores, cmap=SEQUENCIAL, vmin=vmin, vmax=vmax,
                           origin="lower", aspect="auto")
        ax.set_xticks(range(len(tabela.columns)), [f"{v:g}" for v in tabela.columns])
        ax.set_yticks(range(len(tabela.index)), [str(v) for v in tabela.index])
        ax.set_xlabel("taxa de mutação")
        if k == 0:
            ax.set_ylabel("população")
        ax.grid(False)
        for lado in ax.spines.values():
            lado.set_visible(False)
        melhor = np.unravel_index(np.nanargmin(valores), valores.shape)
        for (i, j), v in np.ndenumerate(valores):
            if np.isnan(v):
                continue
            fracao = (v - vmin) / (vmax - vmin) if vmax > vmin else 0
            ax.text(j, i, f"{v:.{casas}f}", ha="center", va="center", fontsize=8.5,
                    color=SUPERFICIE if fracao > 0.55 else TINTA,
                    fontweight="bold" if (i, j) == melhor else "normal")
        ax.set_title(c.upper(), fontsize=11)

    barra = fig.colorbar(imagem, ax=eixos[0].tolist(), shrink=0.9)
    barra.set_label(rotulo_metrica(cenario, tem_ref))
    barra.outline.set_visible(False)
    fig.suptitle(
        f"{estilo.nome_execucao(cenario, n)}: média de {seeds} seeds por célula "
        "(em negrito, a melhor)",
        x=0.01, ha="left", fontsize=12, fontweight="semibold",
    )
    marcar_exemplo(fig, origem)
    fig.savefig(destino)
    plt.close(fig)


def erx_vs_ox(df, tem_ref, destino: Path, origem: Path) -> None:
    combos = sorted(df.groupby(["cenario", "n"]).groups)
    fig, eixos = plt.subplots(1, len(combos), figsize=(4.4 * len(combos), 4.4), squeeze=False)
    for ax, (cenario, n) in zip(eixos[0], combos):
        sub = df[(df["cenario"] == cenario) & (df["n"] == n)]
        cruzamentos = ordenar_cruzamentos(sub["cruzamento"].unique())
        dados, rotulos = [], []
        for c in cruzamentos:
            sc = sub[sub["cruzamento"] == c]
            pop, mut = melhor_config(sc, ["populacao", "taxa_mutacao"])
            sel = sc[(sc["populacao"] == pop) & (sc["taxa_mutacao"] == mut)]
            dados.append(sel["metrica"].to_numpy())
            rotulos.append(f"{c.upper()}\npop. {pop} · mut. {mut:g}")
        posicoes = list(range(1, len(dados) + 1))
        caixas(ax, dados, [cor(c) for c in cruzamentos], posicoes)
        ax.set_xticks(posicoes, rotulos)
        ax.set_xlim(0.4, len(dados) + 0.6)
        ax.set_ylabel(rotulo_metrica(cenario, tem_ref))
        ax.set_title(estilo.nome_execucao(cenario, n), fontsize=11)
    fig.suptitle("ERX × OX: seeds na melhor configuração de cada cruzamento",
                 x=0.01, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    marcar_exemplo(fig, origem)
    fig.savefig(destino)
    plt.close(fig)


def populacao_seeds(df, tem_ref, destino: Path, origem: Path) -> None:
    combos = sorted(df.groupby(["cenario", "n"]).groups)
    fig, eixos = plt.subplots(1, len(combos), figsize=(4.8 * len(combos), 4.4), squeeze=False)
    for ax, (cenario, n) in zip(eixos[0], combos):
        sub = df[(df["cenario"] == cenario) & (df["n"] == n)]
        cruz, _, mut = melhor_config(sub, ["cruzamento", "populacao", "taxa_mutacao"])
        sel = sub[(sub["cruzamento"] == cruz) & (sub["taxa_mutacao"] == mut)]
        pops = sorted(sel["populacao"].unique())
        dados = [sel[sel["populacao"] == p]["metrica"].to_numpy() for p in pops]
        posicoes = list(range(1, len(pops) + 1))
        caixas(ax, dados, [cor(cruz)] * len(pops), posicoes)
        ax.set_xticks(posicoes, [str(p) for p in pops])
        ax.set_xlabel("população")
        ax.set_ylabel(rotulo_metrica(cenario, tem_ref))
        estilo.titulo(ax, estilo.nome_execucao(cenario, n), f"{cruz.upper()}, mutação {mut:g}")
    fig.suptitle("Efeito da população: cada ponto é uma seed",
                 x=0.01, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    marcar_exemplo(fig, origem)
    fig.savefig(destino)
    plt.close(fig)


def tabela_media_desvio(df, tem_ref, destino: Path, origem: Path) -> None:
    def md(serie, casas):
        return f"{serie.mean():.{casas}f} ± {serie.std():.{casas}f}"

    cab = ["Cenário", "N", "Cruzamento", "População", "Mutação", "Seeds", "Melhor distância"]
    if tem_ref:
        cab.append("Gap p/ referência (%)")
    cab += ["Épocas", "Tempo (s)"]
    linhas = []
    for (cenario, n), sub in df.groupby(["cenario", "n"]):
        for c in ordenar_cruzamentos(sub["cruzamento"].unique()):
            sc = sub[sub["cruzamento"] == c]
            pop, mut = melhor_config(sc, ["populacao", "taxa_mutacao"])
            sel = sc[(sc["populacao"] == pop) & (sc["taxa_mutacao"] == mut)]
            linha = [estilo.NOME_CENARIO.get(cenario, cenario), str(n), c.upper(), str(pop),
                     f"{mut:g}", str(len(sel)), md(sel["melhor"], 4)]
            if tem_ref:
                linha.append(md(sel["metrica"], 2))
            linha += [md(sel["epocas"], 0), md(sel["tempo_s"], 3)]
            linhas.append(linha)

    texto = ["# Média ± desvio por cenário", ""]
    if "exemplo" in str(origem).lower():
        texto += ["> **Dados de EXEMPLO (falsos).** Não usar no relatório.", ""]
    texto += [
        f"Gerado por `viz/plotar_experimentos.py` a partir de `{origem.as_posix()}`. "
        "Cada linha usa a melhor configuração (menor média) daquele cruzamento. "
        "Desvio-padrão amostral sobre as seeds.",
        "",
    ]
    if tem_ref:
        texto += [
            "Referência: ótimo no círculo; vizinho mais próximo no uniforme "
            "(negativo = o AG ficou melhor que o vizinho mais próximo).",
            "",
        ]
    texto.append("| " + " | ".join(cab) + " |")
    texto.append("|" + "---|" * len(cab))
    texto += ["| " + " | ".join(l) + " |" for l in linhas]
    destino.write_text("\n".join(texto) + "\n", encoding="utf-8")


# ---------------------------------------------------------------- main

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("csv", type=Path, help="caminho do experimentos.csv")
    parser.add_argument("--saida", type=Path, help="pasta das figuras (padrão: <pasta do csv>/figuras)")
    args = parser.parse_args()

    df, tem_ref = carregar(args.csv)
    pasta = args.saida or args.csv.parent / "figuras"
    pasta.mkdir(parents=True, exist_ok=True)
    feitas = []

    for cenario, n in sorted(df.groupby(["cenario", "n"]).groups):
        nome = f"mapa_calor_{cenario}_n{n}.png"
        mapa_calor(df, cenario, n, tem_ref, pasta / nome, args.csv)
        feitas.append(nome)
    erx_vs_ox(df, tem_ref, pasta / "erx_vs_ox.png", args.csv)
    populacao_seeds(df, tem_ref, pasta / "populacao_seeds.png", args.csv)
    tabela_media_desvio(df, tem_ref, pasta / "media_desvio.md", args.csv)
    feitas += ["erx_vs_ox.png", "populacao_seeds.png", "media_desvio.md"]

    if df["cruzamento"].nunique() < 2:
        print("  aviso: só há um cruzamento no CSV; ERX × OX fica com uma caixa por cenário")
    if not tem_ref:
        print("  aviso: sem a coluna 'referencia'; as figuras usam a melhor distância")
    print(f"  {pasta}: {', '.join(feitas)}")


if __name__ == "__main__":
    main()
