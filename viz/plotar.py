# /// script
# requires-python = ">=3.11"
# dependencies = ["matplotlib>=3.8", "numpy>=1.26", "pandas>=2.1", "pillow>=10"]
# ///
"""Figuras de uma execução do AG: curva por época, grade de rotas, GIF e gap.

Uso (na raiz do repo):
    uv run viz/plotar.py saida/circulo_n50_s1 [outras pastas...] [--gif] [--n-rotas 9]

Lê os arquivos que o `tsp` grava (ver viz/dados.py) e grava em <pasta>/figuras/:
    curva.png        melhor/média/pior e diversidade por época, com as referências
    rotas_grade.png  melhor rota em algumas épocas
    evolucao.gif     animação da melhor rota (com --gif)
    gap.png          gap para o ótimo (ou a referência) por época e por tempo
    marcos.txt       época e tempo até gap <= 5%, <= 1% e até a última melhora
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

import estilo  # liga o backend Agg antes do pyplot
import matplotlib.pyplot as plt
import numpy as np
import referencias
from dados import Execucao, ler_execucao
from estilo import AGUA, AZUL, LARANJA, TINTA_2, TINTA_FRACA
from matplotlib.animation import FuncAnimation, PillowWriter

PISO_GAP = 1e-3  # gap 0 vira 0,001% no eixo log


# ---------------------------------------------------------------- apoio

def alvo(ex: Execucao, refs: dict[str, float]) -> tuple[float, str] | None:
    """Contra o que medir o gap: ótimo do círculo, força bruta ou referência 2-opt."""
    if (otimo := ex.numero("otimo")) is not None:
        return otimo, "ótimo"
    if "forca_bruta" in refs:
        return refs["forca_bruta"], "ótimo (força bruta)"
    if "referencia_2opt" in refs:
        return refs["referencia_2opt"], "referência 2-opt"
    return None


def linhas_referencia(ex: Execucao, refs: dict[str, float]) -> list[tuple[str, float]]:
    linhas = []
    if (otimo := ex.numero("otimo")) is not None:
        linhas.append(("ótimo", otimo))
    for chave, nome in [("forca_bruta", "força bruta (ótimo)"),
                        ("referencia_2opt", "referência 2-opt"),
                        ("vizinho_mais_proximo", "vizinho mais próximo")]:
        if chave in refs:
            linhas.append((nome, refs[chave]))
    return linhas


def eh_exemplo(ex: Execucao) -> bool:
    return ex.resumo.get("origem", "").startswith("exemplo")


def ultima_melhora(ex: Execucao) -> int:
    valor = ex.numero("geracao_ultima_melhora")
    return int(valor) if valor is not None else ex.melhorias[-1].geracao


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
    if n <= 60:
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


def gap_pct(distancia: float, referencia: float) -> float:
    # a rota ótima somada em outra ordem pode ficar 1e-14 abaixo do ótimo
    return max(0.0, (distancia / referencia - 1) * 100)


def texto_rota(geracao, dist, ref) -> str:
    texto = f"época {geracao} · d = {dist:.5g}"
    if ref:
        texto += f" · gap {gap_pct(dist, ref[0]):.2f}%"
    return texto


# ---------------------------------------------------------------- figuras

def plotar_curva(ex: Execucao, refs, nome, destino: Path) -> None:
    h = ex.historico
    fig, (ax, ax_div) = plt.subplots(
        2, 1, figsize=(8, 6.2), sharex=True, gridspec_kw={"height_ratios": [3, 1.1], "hspace": 0.08}
    )
    x = h["geracao"]
    ax.plot(x, h["pior"], color=AGUA, lw=1.5, label="pior", zorder=2)
    ax.plot(x, h["media"], color=LARANJA, lw=1.5, label="média", zorder=3)
    ax.plot(x, h["melhor"], color=AZUL, lw=2, label="melhor", zorder=4)
    if h["pior"].iloc[0] / h["melhor"].min() > 5:
        estilo.eixo_y_log(ax)
    estilo.rotular_referencias(ax, linhas_referencia(ex, refs))

    ultima = ultima_melhora(ex)
    perto_da_borda = ultima > 0.75 * max(int(x.iloc[-1]), 1)
    for eixo in (ax, ax_div):
        eixo.axvline(ultima, color=TINTA_FRACA, lw=1, ls=(0, (1, 2)), zorder=1)
    ax.annotate(
        f"última melhora\népoca {ultima}", xy=(ultima, 1), xycoords=("data", "axes fraction"),
        xytext=(-4 if perto_da_borda else 4, -4), textcoords="offset points",
        ha="right" if perto_da_borda else "left", va="top", fontsize=8.5, color=TINTA_2,
    )

    melhor = h["melhor"].iloc[-1]
    partes = [f"{int(x.iloc[-1])} épocas", f"{h['tempo_s'].iloc[-1]:.2f} s", f"melhor = {melhor:.5g}"]
    if (ref := alvo(ex, refs)) is not None:
        partes.append(f"gap = {gap_pct(melhor, ref[0]):.2f}%")
    if "motivo_parada" in ex.resumo:
        partes.append(f"parada: {ex.resumo['motivo_parada']}")
    estilo.titulo(ax, f"{nome}: distância por época", " · ".join(partes))
    ax.set_ylabel("distância da rota")
    ax.legend(loc="upper right", ncols=3, fontsize=9, bbox_to_anchor=(1, 1.0))

    ax_div.plot(x, h["diversidade"], color=TINTA_2, lw=1.5)
    ax_div.set_ylabel("diversidade")
    ax_div.set_ylim(bottom=0)
    ax_div.set_xlabel("época")
    ax_div.annotate("arestas distintas na população / N (1 = todos iguais)", xy=(1, 1),
                    xycoords="axes fraction", xytext=(-2, -2), textcoords="offset points",
                    ha="right", va="top", fontsize=8, color=TINTA_FRACA)
    estilo.marcar_exemplo(fig, eh_exemplo(ex))
    fig.savefig(destino)
    plt.close(fig)


def plotar_grade(ex: Execucao, refs, nome, destino: Path, k: int) -> None:
    escolhidas = escolher(ex.melhorias, k)  # cada melhoria é uma rota diferente
    colunas = 3 if len(escolhidas) <= 9 else 4
    linhas = math.ceil(len(escolhidas) / colunas)
    fig, eixos = plt.subplots(linhas, colunas, figsize=(3.2 * colunas, 3.4 * linhas + 0.4),
                              squeeze=False)
    s, lw = tamanho_marcas(ex.n)
    ref = alvo(ex, refs)
    for ax, m in zip(eixos.flat, escolhidas):
        preparar_mapa(ax, ex.coords, s)
        desenhar_rota(ax, ex.coords, m.rota, lw)
        ax.set_title(texto_rota(m.geracao, m.distancia, ref), fontsize=9, fontweight="normal",
                     color=TINTA_2)
    for ax in list(eixos.flat)[len(escolhidas):]:
        ax.axis("off")
    fig.suptitle(f"{nome}: melhor rota ao longo das épocas ({len(ex.melhorias)} melhorias no total)",
                 x=0.02, ha="left", fontsize=12, fontweight="semibold")
    fig.tight_layout()
    estilo.marcar_exemplo(fig, eh_exemplo(ex))
    fig.savefig(destino)
    plt.close(fig)


def gerar_gif(ex: Execucao, refs, nome, destino: Path, quadros=80, fps=10) -> None:
    # épocas de 0 até um pouco depois da última melhora, mais densas no começo
    fim = min(int(ex.historico["geracao"].iloc[-1]), int(ultima_melhora(ex) * 1.1) + 1)
    geracoes = sorted(set(np.round(np.linspace(0, 1, quadros) ** 2 * fim).astype(int)))
    escolhidas = [(g, ex.rota_na_geracao(g)) for g in geracoes]
    escolhidas += [escolhidas[-1]] * (2 * fps)  # segura o quadro final por 2 s

    fig, ax = plt.subplots(figsize=(5.5, 5.9), dpi=90)
    s, lw = tamanho_marcas(ex.n)
    preparar_mapa(ax, ex.coords, s)
    ref = alvo(ex, refs)
    linha = desenhar_rota(ax, ex.coords, escolhidas[0][1].rota, lw)
    ax.set_title(nome, fontsize=11)
    estilo.marcar_exemplo(fig, eh_exemplo(ex))
    legenda = ax.annotate("", xy=(0, 0), xycoords="axes fraction", xytext=(0, -14),
                          textcoords="offset points", fontsize=9, color=TINTA_2)

    def atualizar(quadro):
        geracao, m = quadro
        fechada = np.append(m.rota, m.rota[0])
        linha.set_data(ex.coords[fechada, 0], ex.coords[fechada, 1])
        legenda.set_text(texto_rota(geracao, m.distancia, ref))
        return linha, legenda

    animacao = FuncAnimation(fig, atualizar, frames=escolhidas, blit=False)
    animacao.save(destino, writer=PillowWriter(fps=fps), dpi=90)
    plt.close(fig)


def calcular_marcos(ex: Execucao, referencia: float) -> dict[str, str]:
    h = ex.historico
    gap = ((h["melhor"] / referencia - 1) * 100).clip(lower=0)
    marcos = {"referencia": f"{referencia:.6f}"}
    for limite in (5, 1):
        atingiu = h[gap <= limite + 1e-9]
        if len(atingiu):
            marcos[f"epoca_gap_{limite}pct"] = str(int(atingiu["geracao"].iloc[0]))
            marcos[f"tempo_gap_{limite}pct_s"] = f"{atingiu['tempo_s'].iloc[0]:.4f}"
        else:
            marcos[f"epoca_gap_{limite}pct"] = "nao_atingido"
            marcos[f"tempo_gap_{limite}pct_s"] = "nao_atingido"
    marcos["epoca_ultima_melhora"] = str(ultima_melhora(ex))
    tempo_ultima = ex.numero("tempo_ate_ultima_melhora_s")
    if tempo_ultima is None:
        tempo_ultima = float(h.loc[h["geracao"] == ultima_melhora(ex), "tempo_s"].iloc[0])
    marcos["tempo_ultima_melhora_s"] = f"{tempo_ultima:.4f}"
    marcos["gap_final_pct"] = f"{gap.iloc[-1]:.4f}"
    marcos["epocas_total"] = str(int(h["geracao"].iloc[-1]))
    marcos["tempo_total_s"] = f"{h['tempo_s'].iloc[-1]:.4f}"
    return marcos


def plotar_gap(ex: Execucao, ref: tuple[float, str], marcos, nome, destino: Path) -> None:
    h = ex.historico
    gap = ((h["melhor"] / ref[0] - 1) * 100).clip(lower=PISO_GAP)
    fig, eixos = plt.subplots(1, 2, figsize=(10, 4.2), sharey=True)
    for ax, coluna, rotulo in [(eixos[0], "geracao", "época"), (eixos[1], "tempo_s", "tempo (s)")]:
        ax.plot(h[coluna], gap, color=AZUL, lw=2)
        estilo.eixo_y_log(ax)
        if gap.iloc[-1] <= PISO_GAP:
            ax.annotate(f"gap 0 ({ref[1]} exato), desenhado em {PISO_GAP:g}%",
                        xy=(1, PISO_GAP), xycoords=("axes fraction", "data"), xytext=(-2, 5),
                        textcoords="offset points", ha="right", va="bottom", fontsize=8.5,
                        color=TINTA_2)
        ax.set_xlabel(rotulo)
        for limite in (5, 1):
            ax.axhline(limite, color=TINTA_FRACA, lw=1, ls=(0, (4, 3)))
            chave = f"epoca_gap_{limite}pct"
            if marcos[chave] != "nao_atingido":
                epoca = int(marcos[chave])
                xval = h.loc[h["geracao"] == epoca, coluna].iloc[0]
                ax.plot(xval, limite, "o", ms=7, color=AZUL, mec=estilo.SUPERFICIE, mew=2, zorder=5)
                texto = (f"≤ {limite}%: época {epoca}" if coluna == "geracao" else
                         f"≤ {limite}%: {float(marcos[f'tempo_gap_{limite}pct_s']):.2f} s")
                ax.annotate(texto, xy=(xval, limite), xytext=(6, 4), textcoords="offset points",
                            fontsize=8.5, color=TINTA_2)
    eixos[0].set_ylabel(f"gap para o {ref[1]} (%, escala log)" if ref[1] != "referência 2-opt"
                        else "gap para a referência 2-opt (%, log)")
    sub = (f"última melhora: época {marcos['epoca_ultima_melhora']} "
           f"({float(marcos['tempo_ultima_melhora_s']):.2f} s) · "
           f"gap final {float(marcos['gap_final_pct']):.2f}%")
    estilo.titulo(eixos[0], f"{nome}: convergência", sub)
    fig.tight_layout()
    estilo.marcar_exemplo(fig, eh_exemplo(ex))
    fig.savefig(destino)
    plt.close(fig)


# ---------------------------------------------------------------- main

def processar(pasta: Path, gif: bool, n_rotas: int) -> None:
    ex = ler_execucao(pasta)
    refs = referencias.obter(pasta, ex.coords, ex.cenario)
    nome = estilo.nome_execucao(ex.cenario, ex.n, ex.seed)
    figuras = pasta / "figuras"
    figuras.mkdir(exist_ok=True)
    feitas = []

    plotar_curva(ex, refs, nome, figuras / "curva.png")
    feitas.append("curva.png")
    plotar_grade(ex, refs, nome, figuras / "rotas_grade.png", n_rotas)
    feitas.append("rotas_grade.png")
    if gif:
        gerar_gif(ex, refs, nome, figuras / "evolucao.gif")
        feitas.append("evolucao.gif")

    if (ref := alvo(ex, refs)) is not None:
        marcos = calcular_marcos(ex, ref[0])
        (figuras / "marcos.txt").write_text("".join(f"{k}={v}\n" for k, v in marcos.items()),
                                            encoding="utf-8")
        plotar_gap(ex, ref, marcos, nome, figuras / "gap.png")
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
