<#
.SYNOPSIS
    Compila o projeto, roda os cenários do relatório e gera as figuras.

.DESCRIPTION
    1. Compila com CMake + Ninja (g++ do MSYS2) e roda os testes (ctest).
    2. Roda o build/tsp.exe em cada cenário, com os parâmetros padrão do AG:
         - uniforme e círculo com N = 10 e N = 50, seeds 1 a $Seeds
           (N = 50 usa os pontos de --seed-pontos 42, os mesmos das varreduras);
         - círculo N = 100 com mutação 0, 0,5 e 1 (seed 1);
         - bônus: círculo N = 100, 200 e 500 (seed 1), uma execução por vez.
       Execuções que já têm resumo.txt são puladas (use -Refazer para rodar de novo).
    3. Gera as figuras com `uv run`:
         viz/plotar.py              curva, grade de rotas, GIF e gap (seed 1 e bônus)
         viz/tabela_seeds.py        média ± desvio das seeds
         viz/comparar.py            mutação baixa × alta
         viz/plotar_experimentos.py varreduras da Pessoa C (docs/experimentos/)
    4. Copia figuras e tabelas para docs/figuras/ (é para lá que o relatório aponta).

    O experimentos.exe não é rodado aqui: as varreduras levam horas e os CSVs já estão
    em docs/experimentos/ (os comandos estão no README).

    Com -Exemplo: não compila nem roda o C++. Gera dados falsos em saida_exemplo/
    (viz/gerar_exemplo.py) e faz as figuras, sem copiar nada para docs/figuras/.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1 -SemBonus -SemGif
#>
[CmdletBinding()]
param(
    # Usa dados de exemplo em vez do executável C++.
    [switch]$Exemplo,
    # Não gera os GIFs (mais rápido).
    [switch]$SemGif,
    # Não roda o círculo com N grande (~15 min com N = 500).
    [switch]$SemBonus,
    # Roda de novo as execuções que já existem.
    [switch]$Refazer,
    # Argumentos extras passados a todas as execuções do tsp.exe (ex.: '--populacao','200').
    [string[]]$ArgsAG = @(),
    [int]$Seeds = 10,
    [string]$Build = 'build',
    [string]$Saida = 'saida'
)

$ErrorActionPreference = 'Stop'
$Raiz = Split-Path -Parent $PSScriptRoot

# Cenários do relatório: N <= 10 (compara com a força bruta) e N = 50.
$Cenarios = @(
    @{ cenario = 'uniforme'; n = 10 },
    @{ cenario = 'uniforme'; n = 50 },
    @{ cenario = 'circulo'; n = 10 },
    @{ cenario = 'circulo'; n = 50 }
)
# Evidência da mutação (Pessoa C): mesma execução com mutação 0, padrão e 1.
$Mutacoes = @('0', '0.5', '1')
$NMutacao = 100
# Bônus: círculo com N grande.
$Bonus = @(100, 200, 500)

function Invocar {
    # Roda um programa externo e para se ele falhar.
    # No PowerShell 5.1, texto em stderr com 'Stop' vira erro; por isso o 'Continue'.
    param([Parameter(Mandatory)][string]$Programa, [string[]]$Argumentos = @())
    Write-Host "> $Programa $($Argumentos -join ' ')" -ForegroundColor DarkGray
    $anterior = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & $Programa @Argumentos | Out-Host } finally { $ErrorActionPreference = $anterior }
    if ($LASTEXITCODE -ne 0) {
        throw "Falhou (código $LASTEXITCODE): $Programa $($Argumentos -join ' ')"
    }
}

function Exigir([string]$Comando, [string]$Dica) {
    if (-not (Get-Command $Comando -ErrorAction SilentlyContinue)) {
        throw "'$Comando' não encontrado no PATH. $Dica"
    }
}

function Etapa([string]$Texto) {
    Write-Host ""
    Write-Host "== $Texto" -ForegroundColor Cyan
}

function Rodar([string]$Pasta, [string[]]$Argumentos) {
    # Uma execução do tsp; só a última linha de cada época impressa (--log-cada) vai para a tela.
    if (-not $Refazer -and (Test-Path (Join-Path $Pasta 'resumo.txt'))) {
        Write-Host "  já existe: $Pasta" -ForegroundColor DarkGray
        return
    }
    Invocar $script:tsp ($Argumentos + @('--saida', $Pasta, '--log-cada', '1000') + $ArgsAG)
}

Push-Location $Raiz
$relogio = [Diagnostics.Stopwatch]::StartNew()
try {
    Exigir 'uv' 'Instale em https://docs.astral.sh/uv/ (o python do PATH é o atalho da Microsoft Store).'

    if ($Exemplo) {
        $pastaSaida = 'saida_exemplo'
        Etapa "Gerando dados de EXEMPLO em $pastaSaida/"
        if (Test-Path $pastaSaida) { Remove-Item -Recurse -Force $pastaSaida }  # só dados falsos
        Invocar 'uv' @('run', 'viz/gerar_exemplo.py', '--saida', $pastaSaida)
        $pastasFiguras = @(Get-ChildItem $pastaSaida -Directory | ForEach-Object { Join-Path $pastaSaida $_.Name })
        $pastasSeeds = $pastasFiguras
        $pastasMutacao = @()
    }
    else {
        $pastaSaida = $Saida

        Etapa 'Compilando'
        if (Test-Path 'C:\msys64\ucrt64\bin') { $env:PATH = "C:\msys64\ucrt64\bin;$env:PATH" }
        Exigir 'cmake' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-cmake'
        Exigir 'ninja' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-ninja'
        Exigir 'g++' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-gcc'
        Invocar 'cmake' @('-S', '.', '-B', $Build, '-G', 'Ninja')
        Invocar 'cmake' @('--build', $Build)
        Invocar 'ctest' @('--test-dir', $Build, '--output-on-failure')
        $script:tsp = Join-Path $Build 'tsp.exe'

        Etapa "Rodando os cenários do relatório ($Seeds seeds)"
        $pastasFiguras = @()
        $pastasSeeds = @()
        foreach ($c in $Cenarios) {
            foreach ($s in 1..$Seeds) {
                $pasta = Join-Path $pastaSaida "$($c.cenario)_n$($c.n)_s$s"
                Rodar $pasta @('--cenario', $c.cenario, '--n', "$($c.n)", '--seed', "$s")
                $pastasSeeds += $pasta
                if ($s -eq 1) { $pastasFiguras += $pasta }
            }
        }

        Etapa "Rodando o círculo N = $NMutacao com mutação $($Mutacoes -join ', ')"
        $pastasMutacao = @()
        foreach ($m in $Mutacoes) {
            $pasta = Join-Path $pastaSaida "circulo_n${NMutacao}_s1_mut$m"
            Rodar $pasta @('--cenario', 'circulo', '--n', "$NMutacao", '--seed', '1', '--mutacao', $m)
            $pastasMutacao += $pasta
        }

        if (-not $SemBonus) {
            Etapa "Bônus: círculo N = $($Bonus -join ', ')"
            foreach ($n in $Bonus) {
                $pasta = Join-Path $pastaSaida "circulo_n${n}_s1"
                Rodar $pasta @('--cenario', 'circulo', '--n', "$n", '--seed', '1')
                $pastasFiguras += $pasta
            }
        }
    }

    Etapa 'Gerando as figuras'
    $argsPlot = @('run', 'viz/plotar.py') + $pastasFiguras
    if (-not $SemGif) { $argsPlot += '--gif' }
    Invocar 'uv' $argsPlot

    $pastaFigurasGerais = Join-Path $pastaSaida 'figuras'
    Invocar 'uv' (@('run', 'viz/tabela_seeds.py') + $pastasSeeds +
                  @('--saida', (Join-Path $pastaFigurasGerais 'media_desvio.md')))
    if ($pastasMutacao.Count -gt 0) {
        Invocar 'uv' (@('run', 'viz/comparar.py') + $pastasMutacao +
                      @('--saida', (Join-Path $pastaFigurasGerais "mutacao_circulo_n$NMutacao.png"),
                        '--titulo', "Círculo, N = ${NMutacao}: mutação baixa × padrão × alta"))
    }
    if (-not $Exemplo) {
        Invocar 'uv' @('run', 'viz/plotar_experimentos.py', '--saida',
                       (Join-Path $pastaSaida 'experimentos\figuras'))
    }

    if ($Exemplo) {
        Write-Host "Modo -Exemplo: nada é copiado para docs/figuras/."
    }
    else {
        Etapa 'Copiando as figuras para docs/figuras/'
        $destino = Join-Path 'docs' 'figuras'
        $copias = @($pastasFiguras | ForEach-Object {
                @{ de = (Join-Path $_ 'figuras'); para = (Join-Path $destino (Split-Path $_ -Leaf)) } })
        $copias += @{ de = (Join-Path $pastaSaida 'experimentos\figuras'); para = (Join-Path $destino 'experimentos') }
        $copias += @{ de = $pastaFigurasGerais; para = $destino }
        foreach ($c in $copias) {
            if (Test-Path $c.de) {
                New-Item -ItemType Directory -Force $c.para | Out-Null
                Copy-Item (Join-Path $c.de '*') $c.para -Force
                Write-Host "  $($c.de) -> $($c.para)"
            }
        }
    }

    Etapa ("Pronto em {0:N0} s." -f $relogio.Elapsed.TotalSeconds)
}
finally {
    Pop-Location
}
