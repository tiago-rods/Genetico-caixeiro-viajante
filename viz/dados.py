"""Leitura dos arquivos que o `tsp` grava numa pasta de execução.

O formato é o de `src/io/CsvWriter.cpp` (tabela "Arquivos" do README):
    pontos.csv     indice,x,y
    historico.csv  geracao,melhor,media,pior,desvio,diversidade,tempo_ms (toda época)
    rotas.csv      geracao,distancia,tempo_ms,rota (só quando a melhor rota melhora)
    resumo.txt     chave: valor

Os nomes das colunas e das chaves são os do C++. A única coluna acrescentada é
`tempo_s` (= tempo_ms / 1000) no histórico, porque os gráficos usam segundos.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import pandas as pd


@dataclass
class Melhoria:
    geracao: int
    distancia: float
    tempo_s: float
    rota: np.ndarray


@dataclass
class Execucao:
    pasta: Path
    coords: np.ndarray  # (n, 2), na ordem dos índices
    historico: pd.DataFrame
    melhorias: list[Melhoria]  # a primeira é a da geração 0
    resumo: dict[str, str]

    @property
    def cenario(self) -> str:
        return self.resumo.get("cenario", "?")

    @property
    def n(self) -> int:
        return len(self.coords)

    @property
    def seed(self) -> str | None:
        return self.resumo.get("seed")

    def numero(self, chave: str) -> float | None:
        return numero(self.resumo, chave)

    def rota_na_geracao(self, geracao: int) -> Melhoria:
        """Melhor rota da época: a última melhoria com geracao <= época (decisão 005)."""
        geracoes = [m.geracao for m in self.melhorias]
        i = int(np.searchsorted(geracoes, geracao, side="right")) - 1
        return self.melhorias[max(i, 0)]


def ler_resumo(pasta: Path) -> dict[str, str]:
    caminho = pasta / "resumo.txt"
    if not caminho.exists():
        return {}
    resumo = {}
    for linha in caminho.read_text(encoding="utf-8").splitlines():
        if ":" in linha:
            chave, valor = linha.split(":", 1)
            resumo[chave.strip()] = valor.strip()
    return resumo


def numero(resumo: dict[str, str], chave: str) -> float | None:
    try:
        return float(resumo[chave])
    except (KeyError, ValueError):
        return None


def ler_rotas(pasta: Path) -> list[Melhoria]:
    melhorias = []
    with open(pasta / "rotas.csv", encoding="utf-8") as f:
        next(f)  # cabeçalho
        for linha in f:
            linha = linha.strip()
            if linha:
                geracao, dist, tempo_ms, rota = linha.split(",", 3)
                melhorias.append(Melhoria(int(geracao), float(dist), float(tempo_ms) / 1000,
                                          np.array(rota.split(), dtype=int)))
    return melhorias


def ler_historico(pasta: Path) -> pd.DataFrame:
    historico = pd.read_csv(pasta / "historico.csv")
    historico["tempo_s"] = historico["tempo_ms"] / 1000
    return historico


def ler_execucao(pasta: Path) -> Execucao:
    faltando = [nome for nome in ("pontos.csv", "historico.csv", "rotas.csv", "resumo.txt")
                if not (pasta / nome).exists()]
    if faltando:
        raise SystemExit(f"Erro: faltando {', '.join(faltando)} em {pasta}")
    pontos = pd.read_csv(pasta / "pontos.csv").sort_values("indice")
    return Execucao(pasta, pontos[["x", "y"]].to_numpy(), ler_historico(pasta),
                    ler_rotas(pasta), ler_resumo(pasta))
