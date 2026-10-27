# /// script
# requires-python = ">=3.11"
# dependencies = ["matplotlib>=3.8", "numpy>=1.26", "pandas>=2.1"]
# ///
"""Figuras dos experimentos de parâmetros da Pessoa C (`experimentos`).

Uso (na raiz do repo):
    uv run viz/plotar_experimentos.py [CSVs ou pastas...] [--saida PASTA]

Sem argumentos, lê docs/experimentos/*/varredura.csv e */estagnacao.csv.
Grava em --saida (padrão saida/experimentos/figuras):
    mapa_calor_<cenario>_n<N>.png  população × mutação, média das seeds por célula
    populacao_seeds.png            gap de cada seed por população (mutação 0,5)
    estagnacao.png                 perda e acertos por limite de estagnação (população 400)
    erx_vs_ox.png                  só se o CSV tiver mais de um cruzamento (coluna `cruzamento`)

Gap: no círculo, o `gap_pct` do CSV (contra o ótimo). No uniforme, recalculado contra a
referência 2-opt, como nas tabelas do README; o `gap_pct` do CSV usa a melhor distância
do próprio experimento, que muda de um arquivo para outro.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import estilo  # liga o backend Agg antes do pyplot
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from estilo import AGUA, AZUL, COR_CRUZAMENTO, LARANJA, SEQUENCIAL, SUPERFICIE, TINTA, TINTA_2, TINTA_FRACA

RAIZ = Path(__file__).resolve().parent.parent

# Referência 2-opt + Or-opt (scripts/referencia_2opt.py, 300 reinícios) das instâncias
# uniformes do `experimentos` (--seed-pontos 42). Os mesmos valores do README.
REFERENCIA_2OPT_UNIFORME = {50: 616.535026766, 100: 821.640410398}

TOLERANCIA_OTIMO = 1e-6  # gap (%) abaixo disso conta como ótimo atingido (kToleranciaOtimo)
MUTACAO_PADRAO = 0.5
POPULACAO_PADRAO = 400


# ---------------------------------------------------------------- dados

def achar_csvs(entradas: list[Path], nome: str) -> list[Path]:
    if not entradas:
        return sorted((RAIZ / "docs" / "experimentos").glob(f"*/{nome}"))
    achados = []
    for e in entradas:
        if e.is_dir():
            achados += sorted(e.glob(f"**/{nome}"))
        elif e.name == nome:
            achados.append(e)
    return achados


def carregar_varredura(caminhos: list[Path]) -> pd.DataFrame:
    df = pd.concat([pd.read_csv(c) for c in caminhos], ignore_index=True)
    if "cruzamento" not in df.columns:
        df["cruzamento"] = "erx"  # o `experimentos` só roda o ERX
    df["cruzamento"] = df["cruzamento"].str.lower()
    chaves = ["cenario", "n", "cruzamento", "populacao", "taxa_mutacao", "seed"]
    df = df.drop_duplicates(chaves).copy()

    df["gap"] = df["gap_pct"]
    for n, ref in REFERENCIA_2OPT_UNIFORME.items():
        sel = (df["cenario"] == "uniforme") & (df["n"] == n)
        df.loc[sel, "gap"] = ((df.loc[sel, "melhor_distancia"] / ref - 1) * 100).clip(lower=0)
    sem_ref = df[(df["cenario"] == "uniforme") & ~df["n"].isin(list(REFERENCIA_2OPT_UNIFORME))]
    if len(sem_ref):
        print(f"  aviso: uniforme com N = {sorted(sem_ref['n'].unique())} sem referência 2-opt; "
              "usando o gap_pct do CSV")
    df["atingiu"] = df["gap_pct"] < TOLERANCIA_OTIMO
    return df


def rotulo_gap(cenario: str) -> str:
    return "gap para o ótimo (%)" if cenario == "circulo" else "gap para a referência 2-opt (%)"


def ordenar_cruzamentos(cruzamentos) -> list[str]:
    presentes = set(cruzamentos)
    conhecidos = [c for c in COR_CRUZAMENTO if c in presentes]
    return conhecidos + sorted(presentes - set(conhecidos))


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

def mapa_calor(df, cenario, n, destino: Path) -> None:
    """Cor = gap médio. No círculo, o texto é quantas seeds acertaram o ótimo."""
    sub = df[(df["cenario"] == cenario) & (df["n"] == n) & (df["cruzamento"] == "erx")]
    gap = sub.pivot_table(index="populacao", columns="taxa_mutacao", values="gap", aggfunc="mean")
    acertos = sub.pivot_table(index="populacao", columns="taxa_mutacao", values="atingiu", aggfunc="sum")
    seeds = sub.groupby(["populacao", "taxa_mutacao"]).size().max()
    valores = gap.to_numpy()
    vmin, vmax = np.nanmin(valores), np.nanmax(valores)
    norma = plt.matplotlib.colors.PowerNorm(0.5, vmin=0, vmax=vmax)  # gaps grandes não apagam o resto

    fig, ax = plt.subplots(figsize=(1.05 * len(gap.columns) + 2.6, 0.62 * len(gap.index) + 1.8),
                           constrained_layout=True)
    imagem = ax.imshow(valores, cmap=SEQUENCIAL, norm=norma, origin="lower", aspect="auto")
    ax.set_xticks(range(len(gap.columns)), [f"{v:g}" for v in gap.columns])
    ax.set_yticks(range(len(gap.index)), [str(v) for v in gap.index])
    ax.set_xlabel("taxa de mutação")
    ax.set_ylabel("população")
    ax.grid(False)
    for lado in ax.spines.values():
        lado.set_visible(False)
    for (i, j), v in np.ndenumerate(valores):
        if np.isnan(v):
            continue
        escuro = norma(v) > 0.55
        pop, mut = gap.index[i], gap.columns[j]
        padrao = pop == POPULACAO_PADRAO and mut == MUTACAO_PADRAO
        if cenario == "circulo":
            texto = f"{int(acertos.iloc[i, j])}/{seeds}"
            if acertos.iloc[i, j] < seeds:
                texto += f"\n({v:.1f}%)"
        else:
            texto = f"{v:.1f}"
        ax.text(j, i, texto, ha="center", va="center", fontsize=8.5 if "\n" not in texto else 7.5,
                color=SUPERFICIE if escuro else TINTA, fontweight="bold" if padrao else "normal")
        if padrao:
            ax.add_patch(plt.Rectangle((j - 0.5, i - 0.5), 1, 1, fill=False, ec=LARANJA, lw=2))

    barra = fig.colorbar(imagem, ax=ax, shrink=0.9)
    barra.set_label(f"{rotulo_gap(cenario)}, média")
    barra.outline.set_visible(False)
    sub_titulo = (f"média de {seeds} seeds por célula; no círculo, quantas acertaram o ótimo"
                  if cenario == "circulo" else f"gap médio de {seeds} seeds por célula")
    estilo.titulo(ax, f"{estilo.nome_execucao(cenario, n)}: população × mutação",
                  f"{sub_titulo} · contorno: padrão (pop. {POPULACAO_PADRAO}, mut. {MUTACAO_PADRAO:g})")
    fig.savefig(destino)
    plt.close(fig)


def populacao_seeds(df, destino: Path) -> None:
    # só o uniforme: no círculo quase todas as seeds ficam em gap 0 (ver o mapa de calor)
    sel = df[(df["taxa_mutacao"] == MUTACAO_PADRAO) & (df["cruzamento"] == "erx")
             & (df["cenario"] == "uniforme")]
    combos = sorted(sel.groupby(["cenario", "n"]).groups, key=lambda c: (c[0] != "uniforme", c[1]))
    fig, eixos = plt.subplots(1, len(combos), figsize=(4.2 * len(combos), 4.4), squeeze=False)
    for ax, (cenario, n) in zip(eixos[0], combos):
        sub = sel[(sel["cenario"] == cenario) & (sel["n"] == n)]
        pops = sorted(sub["populacao"].unique())
        dados = [sub[sub["populacao"] == p]["gap"].to_numpy() for p in pops]
        cores = [LARANJA if p == POPULACAO_PADRAO else AZUL for p in pops]
        posicoes = list(range(1, len(pops) + 1))
        caixas(ax, dados, cores, posicoes)
        ax.set_xticks(posicoes, [str(p) for p in pops])
        ax.set_xlabel("população")
        ax.set_ylabel(rotulo_gap(cenario))
        ax.set_ylim(bottom=min(-0.5, ax.get_ylim()[0]))
        ax.set_title(estilo.nome_execucao(cenario, n), fontsize=11)
    fig.suptitle(f"Efeito da população com mutação {MUTACAO_PADRAO:g}: cada ponto é uma seed "
                 f"(em laranja, o padrão {POPULACAO_PADRAO})",
                 x=0.01, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    fig.savefig(destino)
    plt.close(fig)


def estagnacao(caminhos: list[Path], destino: Path) -> None:
    df = pd.concat([pd.read_csv(c) for c in caminhos], ignore_index=True)
    df = df[df["populacao"] == POPULACAO_PADRAO]
    df["sem_perda"] = df["perda_pct"] < TOLERANCIA_OTIMO
    resumo = df.groupby(["cenario", "n", "estagnacao"]).agg(
        perda=("perda_pct", "mean"), sem_perda=("sem_perda", "sum"), execucoes=("seed", "size"),
        parada=("geracao_parada", "mean")).reset_index()

    fig, (ax_u, ax_c) = plt.subplots(1, 2, figsize=(11, 4.4))
    cores = {50: AZUL, 100: LARANJA, 200: AGUA}
    for n, sub in resumo[resumo["cenario"] == "uniforme"].groupby("n"):
        ax_u.plot(sub["estagnacao"], sub["perda"], "o-", color=cores.get(n, TINTA_2), ms=4,
                  label=f"N = {n}")
    for n, sub in resumo[resumo["cenario"] == "circulo"].groupby("n"):
        ax_c.plot(sub["estagnacao"], sub["sem_perda"], "o-", color=cores.get(n, TINTA_2), ms=4,
                  label=f"N = {n}")
    for ax in (ax_u, ax_c):
        ax.set_xscale("log")
        ax.set_xticks([25, 50, 100, 200, 300, 600, 1000, 2000],
                      ["25", "50", "100", "200", "300", "600", "1000", "2000"])
        ax.minorticks_off()
        ax.set_xlabel("limite de estagnação (gerações sem melhora)")
        for limite, texto in [(300, "padrão até N = 100"), (600, "padrão N = 200 (3·N)")]:
            ax.axvline(limite, color=TINTA_FRACA, lw=1, ls=(0, (4, 3)), zorder=1)
            ax.annotate(texto, xy=(limite, 0), xycoords=("data", "axes fraction"), xytext=(3, 4),
                        textcoords="offset points", fontsize=8, color=TINTA_2, va="bottom", rotation=90)
    ax_u.legend(loc="upper right", fontsize=9)
    ax_c.legend(loc="lower right", fontsize=9)
    execucoes = int(resumo["execucoes"].max())
    ax_c.set_ylim(-0.3, execucoes + 0.5)
    ax_u.set_ylim(bottom=0)
    ax_u.set_ylabel("perda média (%)")
    ax_c.set_ylabel(f"execuções sem perda (de {execucoes})")
    estilo.titulo(ax_u, "Uniforme: perda ao parar", "rota no ponto de parada vs. a mesma execução longa")
    estilo.titulo(ax_c, "Círculo: paradas no ótimo", "todas as execuções longas chegam ao ótimo exato")
    fig.suptitle(f"Critério de parada (população {POPULACAO_PADRAO}, mutação {MUTACAO_PADRAO:g})",
                 x=0.01, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    fig.savefig(destino)
    plt.close(fig)


def erx_vs_ox(df, destino: Path) -> None:
    combos = sorted(df.groupby(["cenario", "n"]).groups)
    fig, eixos = plt.subplots(1, len(combos), figsize=(4.4 * len(combos), 4.4), squeeze=False)
    for ax, (cenario, n) in zip(eixos[0], combos):
        sub = df[(df["cenario"] == cenario) & (df["n"] == n)]
        cruzamentos = ordenar_cruzamentos(sub["cruzamento"].unique())
        dados, rotulos = [], []
        for c in cruzamentos:
            sc = sub[sub["cruzamento"] == c]
            pop, mut = sc.groupby(["populacao", "taxa_mutacao"])["gap"].mean().idxmin()
            sel = sc[(sc["populacao"] == pop) & (sc["taxa_mutacao"] == mut)]
            dados.append(sel["gap"].to_numpy())
            rotulos.append(f"{c.upper()}\npop. {pop} · mut. {mut:g}")
        posicoes = list(range(1, len(dados) + 1))
        caixas(ax, dados, [COR_CRUZAMENTO.get(c, AGUA) for c in cruzamentos], posicoes)
        ax.set_xticks(posicoes, rotulos)
        ax.set_xlim(0.4, len(dados) + 0.6)
        ax.set_ylabel(rotulo_gap(cenario))
        ax.set_title(estilo.nome_execucao(cenario, n), fontsize=11)
    fig.suptitle("ERX × OX: seeds na melhor configuração de cada cruzamento",
                 x=0.01, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    fig.savefig(destino)
    plt.close(fig)


# ---------------------------------------------------------------- main

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("entradas", nargs="*", type=Path,
                        help="varredura.csv, estagnacao.csv ou pastas (padrão: docs/experimentos)")
    parser.add_argument("--saida", type=Path, default=Path("saida/experimentos/figuras"),
                        help="pasta das figuras (padrão: saida/experimentos/figuras)")
    args = parser.parse_args()
    args.saida.mkdir(parents=True, exist_ok=True)
    feitas = []

    varreduras = achar_csvs(args.entradas, "varredura.csv")
    if varreduras:
        df = carregar_varredura(varreduras)
        for cenario, n in sorted(df.groupby(["cenario", "n"]).groups):
            nome = f"mapa_calor_{cenario}_n{n}.png"
            mapa_calor(df, cenario, n, args.saida / nome)
            feitas.append(nome)
        populacao_seeds(df, args.saida / "populacao_seeds.png")
        feitas.append("populacao_seeds.png")
        if df["cruzamento"].nunique() >= 2:
            erx_vs_ox(df, args.saida / "erx_vs_ox.png")
            feitas.append("erx_vs_ox.png")
        else:
            print("  aviso: só o ERX nos CSVs (o OX não existe ainda); erx_vs_ox.png não foi gerado")
    else:
        print("  aviso: nenhum varredura.csv encontrado")

    estagnacoes = achar_csvs(args.entradas, "estagnacao.csv")
    if estagnacoes:
        estagnacao(estagnacoes, args.saida / "estagnacao.png")
        feitas.append("estagnacao.png")

    print(f"  {args.saida}: {', '.join(feitas)}")


if __name__ == "__main__":
    main()
