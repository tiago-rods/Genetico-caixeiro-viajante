# /// script
# requires-python = ">=3.11"
# dependencies = ["matplotlib>=3.8", "numpy>=1.26", "pandas>=2.1"]
# ///
"""Compara várias execuções do `tsp` no mesmo gráfico (ex.: mutação baixa × alta).

Uso (na raiz do repo):
    uv run viz/comparar.py saida/circulo_n100_s1_mut0 saida/circulo_n100_s1_mut0.5 ... \
        --saida saida/figuras/mutacao.png [--titulo "..."]

Painel de cima: melhor distância por época (gap em %, escala log, se houver ótimo).
Painel de baixo: diversidade da população. Cada execução é rotulada pela taxa de
mutação do resumo.txt (ou pelo nome da pasta, se a mutação for igual em todas).
"""

from __future__ import annotations

import argparse
from pathlib import Path

import estilo  # liga o backend Agg antes do pyplot
import matplotlib.pyplot as plt
from dados import ler_execucao
from estilo import AGUA, AZUL, LARANJA, TINTA_2, TINTA_FRACA

CORES = [AGUA, AZUL, LARANJA, TINTA_2]
PISO_GAP = 1e-3


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pastas", nargs="+", type=Path)
    parser.add_argument("--saida", type=Path, required=True)
    parser.add_argument("--titulo", default=None)
    args = parser.parse_args()

    execucoes = [ler_execucao(p) for p in args.pastas]
    mutacoes = [ex.resumo.get("taxa_mutacao") for ex in execucoes]
    rotulos = ([f"mutação {m}" for m in mutacoes] if len(set(mutacoes)) == len(mutacoes)
               else [ex.pasta.name for ex in execucoes])
    otimo = execucoes[0].numero("otimo")

    fig, (ax, ax_div) = plt.subplots(2, 1, figsize=(8.5, 6.4), sharex=True,
                                     gridspec_kw={"height_ratios": [2.2, 1], "hspace": 0.08})
    for ex, rotulo, cor in zip(execucoes, rotulos, CORES):
        h = ex.historico
        y = ((h["melhor"] / otimo - 1) * 100).clip(lower=PISO_GAP) if otimo else h["melhor"]
        final = (f"gap final {max(0, (h['melhor'].iloc[-1] / otimo - 1) * 100):.2f}%" if otimo
                 else f"melhor {h['melhor'].iloc[-1]:.1f}")
        ax.plot(h["geracao"], y, color=cor, lw=2,
                label=f"{rotulo}: {final}, parou na época {int(h['geracao'].iloc[-1])}")
        ax_div.plot(h["geracao"], h["diversidade"], color=cor, lw=1.6)
        ultima = ex.numero("geracao_ultima_melhora")
        if ultima is not None:
            ax.axvline(ultima, color=cor, lw=1, ls=(0, (1, 2)), alpha=0.8)

    if otimo:
        estilo.eixo_y_log(ax)
        ax.set_ylabel("gap para o ótimo (%, log)")
    else:
        ax.set_ylabel("melhor distância")
    ax.legend(loc="upper right", fontsize=8.5)
    primeira = execucoes[0]
    estilo.titulo(ax, args.titulo or f"{estilo.nome_execucao(primeira.cenario, primeira.n)}: comparação",
                  f"população {primeira.resumo.get('populacao')}, seed {primeira.seed} · "
                  "pontilhado: última melhora de cada execução")
    ax_div.set_ylabel("diversidade")
    ax_div.set_xlabel("época")
    ax_div.set_ylim(bottom=0)
    ax_div.axhline(1, color=TINTA_FRACA, lw=1, ls=(0, (4, 3)))
    ax_div.annotate("1 = população toda igual", xy=(1, 1), xycoords=("axes fraction", "data"),
                    xytext=(-2, 3), textcoords="offset points", ha="right", va="bottom",
                    fontsize=8, color=TINTA_FRACA)

    args.saida.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.saida)
    plt.close(fig)
    print(f"  {args.saida}")


if __name__ == "__main__":
    main()
