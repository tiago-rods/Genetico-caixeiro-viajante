# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy>=1.26", "pandas>=2.1"]
# ///
"""Tabela média ± desvio de várias seeds do `tsp`, pronta para o relatório.

Uso (na raiz do repo):
    uv run viz/tabela_seeds.py saida/uniforme_n10_s* saida/circulo_n50_s* ... [--saida arquivo.md]

Agrupa as pastas por cenário, N e parâmetros (tudo do resumo.txt menos a seed) e grava
uma tabela Markdown (padrão: saida/media_desvio.md). A referência de cada cenário é a
mesma dos gráficos (viz/referencias.py): ótimo no círculo, força bruta com N <= 10,
2-opt no uniforme maior; o vizinho mais próximo aparece como comparação.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd
import referencias
from dados import ler_resumo

TOLERANCIA_OTIMO = 1e-6  # mesma do experimentos.cpp
PARAMETROS = ["cenario", "pontos", "seed_pontos", "populacao", "taxa_mutacao", "taxa_cruzamento",
              "torneio", "elite", "max_estagnacao"]


def linha_execucao(pasta: Path) -> dict:
    resumo = ler_resumo(pasta)
    coords = pd.read_csv(pasta / "pontos.csv").sort_values("indice")[["x", "y"]].to_numpy()
    refs = referencias.obter(pasta, coords, resumo["cenario"])
    if "otimo" in resumo:
        ref, nome_ref = float(resumo["otimo"]), "ótimo"
    elif "forca_bruta" in refs:
        ref, nome_ref = refs["forca_bruta"], "força bruta"
    else:
        ref, nome_ref = refs.get("referencia_2opt", np.nan), "2-opt"
    melhor = float(resumo["melhor_distancia"])
    linha = {k: resumo.get(k) for k in PARAMETROS}
    linha.update(
        seed=resumo["seed"], melhor=melhor, referencia=ref, nome_ref=nome_ref,
        gap=max(0.0, (melhor / ref - 1) * 100),
        vizinho=refs["vizinho_mais_proximo"],
        vs_vizinho=(melhor / refs["vizinho_mais_proximo"] - 1) * 100,
        geracoes=int(resumo["geracoes"]), ultima=int(resumo["geracao_ultima_melhora"]),
        tempo=float(resumo["tempo_total_s"]),
    )
    return linha


def md(serie: pd.Series, casas: int) -> str:
    return f"{serie.mean():.{casas}f} ± {serie.std():.{casas}f}"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("pastas", nargs="+", type=Path)
    parser.add_argument("--saida", type=Path, default=Path("saida/media_desvio.md"))
    args = parser.parse_args()

    df = pd.DataFrame([linha_execucao(p) for p in args.pastas if (p / "resumo.txt").exists()])
    df["pontos"] = df["pontos"].astype(int)
    df = df.sort_values(["cenario", "pontos"], key=lambda c: c.map({"uniforme": 0, "circulo": 1})
                        if c.name == "cenario" else c)

    cab = ["Cenário", "N", "Seeds", "Melhor distância", "Referência", "Gap (%)",
           "Atingiu a ref.", "Vizinho mais próximo", "AG vs. vizinho (%)", "Gerações",
           "Última melhora", "Tempo (s)"]
    linhas = []
    for _, g in df.groupby(PARAMETROS, sort=False, dropna=False):
        cenario = "Círculo" if g["cenario"].iloc[0] == "circulo" else "Uniforme"
        linhas.append([
            cenario, str(g["pontos"].iloc[0]), str(len(g)), md(g["melhor"], 2),
            f"{g['referencia'].iloc[0]:.2f} ({g['nome_ref'].iloc[0]})", md(g["gap"], 2),
            f"{int((g['gap'] < TOLERANCIA_OTIMO).sum())}/{len(g)}",
            f"{g['vizinho'].iloc[0]:.2f}", md(g["vs_vizinho"], 1), md(g["geracoes"], 0),
            md(g["ultima"], 0), md(g["tempo"], 2),
        ])

    p = df.iloc[0]
    texto = [
        f"Média ± desvio-padrão amostral sobre as seeds do AG (os pontos são os mesmos em todas). "
        f"Parâmetros: população {p['populacao']}, mutação {p['taxa_mutacao']}, cruzamento "
        f"{p['taxa_cruzamento']}, torneio {p['torneio']}, elite {p['elite']}. Gap contra a referência; "
        "\"AG vs. vizinho\" negativo = o AG ficou abaixo (melhor) que o vizinho mais próximo.",
        "",
        "| " + " | ".join(cab) + " |",
        "|" + "---|" * len(cab),
        *["| " + " | ".join(l) + " |" for l in linhas],
    ]
    args.saida.parent.mkdir(parents=True, exist_ok=True)
    args.saida.write_text("\n".join(texto) + "\n", encoding="utf-8")
    print(f"  {args.saida}: {len(linhas)} cenários, {len(df)} execuções")


if __name__ == "__main__":
    main()
