"""Cores e estilo comuns aos gráficos de viz/.

Importar antes do pyplot: `import estilo` já liga o backend sem janela (Agg).
As 3 cores de série foram validadas para daltonismo (todos os pares).
"""

from __future__ import annotations

import matplotlib

matplotlib.use("Agg")

import math  # noqa: E402

from cycler import cycler  # noqa: E402
from matplotlib.colors import LinearSegmentedColormap  # noqa: E402
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter  # noqa: E402

SUPERFICIE = "#fcfcfb"
TINTA = "#0b0b0b"  # títulos e valores
TINTA_2 = "#52514e"  # rótulos e textos secundários
TINTA_FRACA = "#898781"  # linhas de referência, pontos
GRADE = "#e1e0d9"
EIXO = "#c3c2b7"

AZUL, LARANJA, AGUA = "#2a78d6", "#eb6834", "#1baf7a"
SERIES = [AZUL, LARANJA, AGUA]

# Cor fixa por cruzamento: a cor segue o operador, não a posição no gráfico.
COR_CRUZAMENTO = {"erx": AZUL, "ox": LARANJA}

# Rampa sequencial azul (claro = pouco, escuro = muito).
SEQUENCIAL = LinearSegmentedColormap.from_list(
    "azul",
    ["#cde2fb", "#9ec5f4", "#6da7ec", "#3987e5", "#256abf", "#184f95", "#0d366b"],
)

NOME_CENARIO = {"uniforme": "Uniforme", "circulo": "Círculo"}

matplotlib.rcParams.update(
    {
        "figure.facecolor": SUPERFICIE,
        "axes.facecolor": SUPERFICIE,
        "savefig.facecolor": SUPERFICIE,
        "savefig.dpi": 150,
        "savefig.bbox": "tight",
        "font.family": "sans-serif",
        "font.sans-serif": ["Segoe UI", "Helvetica", "Arial", "DejaVu Sans"],
        "font.size": 10,
        "text.color": TINTA,
        "axes.edgecolor": EIXO,
        "axes.labelcolor": TINTA_2,
        "axes.titlecolor": TINTA,
        "axes.titlesize": 12,
        "axes.titleweight": "semibold",
        "axes.titlelocation": "left",
        "axes.spines.top": False,
        "axes.spines.right": False,
        "axes.grid": True,
        "axes.axisbelow": True,
        "axes.prop_cycle": cycler(color=SERIES),
        "grid.color": GRADE,
        "grid.linewidth": 0.8,
        "grid.linestyle": "-",
        "xtick.color": EIXO,
        "ytick.color": EIXO,
        "xtick.labelcolor": TINTA_2,
        "ytick.labelcolor": TINTA_2,
        "lines.linewidth": 2,
        "legend.frameon": False,
        "legend.labelcolor": TINTA_2,
    }
)


def titulo(ax, principal: str, sub: str | None = None) -> None:
    """Título à esquerda, com uma linha de contexto menor embaixo."""
    ax.set_title(principal, pad=22 if sub else 8)
    if sub:
        ax.annotate(
            sub,
            xy=(0, 1),
            xycoords="axes fraction",
            xytext=(0, 6),
            textcoords="offset points",
            va="bottom",
            fontsize=9,
            color=TINTA_2,
        )


def eixo_y_log(ax) -> None:
    """Eixo y em log com números simples (2, 5, 10...) em vez de 2×10¹."""
    ax.set_yscale("log")
    baixo, alto = ax.get_ylim()
    decadas = math.log10(alto / baixo)
    ax.yaxis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0) if decadas <= 3 else (1.0,)))
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    ax.yaxis.set_minor_formatter(NullFormatter())


def rotular_referencias(ax, refs: list[tuple[str, float]]) -> None:
    """Linhas horizontais de referência, rotuladas à direita, fora do gráfico.

    Valores iguais viram um rótulo só; rótulos próximos são afastados.
    """
    juntas: list[tuple[str, float]] = []
    for nome, valor in sorted(refs, key=lambda r: r[1]):
        if juntas and abs(valor - juntas[-1][1]) <= 1e-6 * abs(valor):
            juntas[-1] = (f"{juntas[-1][0]} e {nome}", juntas[-1][1])
        else:
            juntas.append((nome, valor))
    for _, valor in juntas:
        ax.axhline(valor, color=TINTA_FRACA, lw=1, ls=(0, (4, 3)), zorder=1)

    ax.get_ylim()  # força o autoscale antes de converter coordenadas
    dpi = ax.figure.dpi
    espaco = 12 * dpi / 72  # 12 pt entre rótulos
    anterior = -math.inf
    for nome, valor in juntas:
        y = ax.transData.transform((0, valor))[1]
        y_rotulo = max(y, anterior + espaco)
        anterior = y_rotulo
        ax.annotate(
            f"{nome} = {valor:.4g}",
            xy=(1, valor),
            xycoords=("axes fraction", "data"),
            xytext=(6, (y_rotulo - y) * 72 / dpi),
            textcoords="offset points",
            ha="left",
            va="center",
            fontsize=8.5,
            color=TINTA_2,
        )


def marcar_exemplo(fig, ativo: bool = True) -> None:
    """Carimbo no canto superior direito das figuras feitas com dados falsos."""
    if ativo:
        fig.text(0.995, 0.995, "DADOS DE EXEMPLO (falsos)", ha="right", va="top",
                 fontsize=8, color=TINTA_FRACA)


def nome_execucao(cenario: str | None, n, seed=None) -> str:
    nome = NOME_CENARIO.get(cenario or "", cenario or "?")
    texto = f"{nome}, N = {n}"
    return f"{texto} (seed {seed})" if seed not in (None, "") else texto
