<#
.SYNOPSIS
    Compila o projeto, roda os cenários e gera as figuras.

.DESCRIPTION
    Sem -Exemplo: compila com CMake + Ninja (g++ do MSYS2), roda cada cenário
    com o executável principal, roda a varredura de experimentos e gera as
    figuras com `uv run`. Tudo vai para saida/, e as figuras são copiadas
    para docs/figuras/ (é para lá que o relatório aponta).

    Com -Exemplo: não compila nem roda o C++. Gera dados falsos em
    saida_exemplo/ (viz/gerar_exemplo.py) e faz as figuras. Serve para testar
    os gráficos antes de o executável existir.

    Executáveis esperados (proposta em docs/divisao-trabalho.md):
      build/tsp.exe           um cenário por execução (src/main.cpp)
      build/experimentos.exe  varredura; grava <saida>/experimentos.csv

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1 -Exemplo

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\rodar_tudo.ps1 -SemGif -ArgsAG '--populacao','200'
#>
[CmdletBinding()]
param(
    # Usa dados de exemplo em vez do executável C++.
    [switch]$Exemplo,
    # Não gera os GIFs (mais rápido).
    [switch]$SemGif,
    # Não roda o círculo com N grande.
    [switch]$SemBonus,
    # Não roda a varredura de experimentos.
    [switch]$SemExperimentos,
    # Argumentos extras passados a todas as execuções do tsp.exe.
    [string[]]$ArgsAG = @(),
    [int]$Seed = 1,
    [string]$Build = 'build',
    [string]$Saida = 'saida'
)

$ErrorActionPreference = 'Stop'
$Raiz = Split-Path -Parent $PSScriptRoot

# Cenários do relatório: N <= 10 (compara com força bruta) e N ~ 30-50.
$Cenarios = @(
    @{ cenario = 'uniforme'; n = 10 },
    @{ cenario = 'uniforme'; n = 40 },
    @{ cenario = 'circulo'; n = 10 },
    @{ cenario = 'circulo'; n = 40 }
)
# Bônus: círculo com N grande; rotas.csv só a cada k épocas.
$Bonus = @(
    @{ cenario = 'circulo'; n = 100; intervalo = 5 },
    @{ cenario = 'circulo'; n = 500; intervalo = 20 },
    @{ cenario = 'circulo'; n = 1000; intervalo = 50 }
)
# Evidência da Pessoa C: a mesma execução com mutação baixa e alta.
# Os valores são provisórios; a Pessoa C ajusta.
$Mutacao = @(
    @{ cenario = 'circulo'; n = 40; sufixo = 'mutbaixa'; extra = @('--taxa-mutacao', '0.01') },
    @{ cenario = 'circulo'; n = 40; sufixo = 'mutalta'; extra = @('--taxa-mutacao', '0.5') }
)

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

Push-Location $Raiz
$relogio = [Diagnostics.Stopwatch]::StartNew()
try {
    Exigir 'uv' 'Instale em https://docs.astral.sh/uv/ (o python do PATH é o atalho da Microsoft Store).'

    if ($Exemplo) {
        $pastaSaida = 'saida_exemplo'
        Etapa "Gerando dados de EXEMPLO em $pastaSaida/"
        Invocar 'uv' @('run', 'viz/gerar_exemplo.py', '--saida', $pastaSaida)
    }
    else {
        $pastaSaida = $Saida

        Etapa 'Compilando'
        if (-not (Select-String -Path 'CMakeLists.txt' -Pattern 'add_executable' -Quiet)) {
            throw ("CMakeLists.txt ainda não define executáveis (bloco da Pessoa A). " +
                   "Para testar só os gráficos: rodar_tudo.ps1 -Exemplo")
        }
        Exigir 'cmake' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-cmake'
        Exigir 'ninja' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-ninja'
        Exigir 'g++' 'Instale pelo MSYS2: pacman -S mingw-w64-ucrt-x86_64-gcc'
        Invocar 'cmake' @('-S', '.', '-B', $Build, '-G', 'Ninja',
                          '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=g++')
        Invocar 'cmake' @('--build', $Build)

        $tsp = Join-Path $Build 'tsp.exe'
        if (-not (Test-Path $tsp)) {
            throw "Não achei $tsp. Confira o nome do executável no CMakeLists.txt (proposta: tsp)."
        }

        Etapa 'Rodando os cenários'
        $lista = $Cenarios + $Mutacao
        if (-not $SemBonus) { $lista += $Bonus }
        foreach ($c in $lista) {
            $nome = "$($c.cenario)_n$($c.n)_s$Seed"
            if ($c.sufixo) { $nome += "_$($c.sufixo)" }
            $pasta = Join-Path $pastaSaida $nome
            $argumentos = @('--cenario', $c.cenario, '--n', "$($c.n)", '--seed', "$Seed",
                            '--saida', $pasta)
            if ($c.intervalo) { $argumentos += @('--intervalo-rotas', "$($c.intervalo)") }
            $argumentos += $ArgsAG
            if ($c.extra) { $argumentos += $c.extra }  # por último, para valer sobre ArgsAG
            Invocar $tsp $argumentos
        }

        if (-not $SemExperimentos) {
            Etapa 'Rodando a varredura de experimentos'
            $exp = Join-Path $Build 'experimentos.exe'
            if (Test-Path $exp) {
                Invocar $exp @('--saida', $pastaSaida)
            }
            else {
                Write-Warning "Não achei $exp (bloco da Pessoa C). Pulando a varredura."
            }
        }
    }

    Etapa 'Gerando as figuras'
    $pastas = @(Get-ChildItem -Path $pastaSaida -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName 'historico.csv') } |
        ForEach-Object { Join-Path $pastaSaida $_.Name })
    if ($pastas.Count -eq 0) { throw "Nenhuma execução (historico.csv) em $pastaSaida/." }
    $argsPlot = @('run', 'viz/plotar.py') + $pastas
    if (-not $SemGif) { $argsPlot += '--gif' }
    Invocar 'uv' $argsPlot

    $csv = Join-Path $pastaSaida 'experimentos.csv'
    if (Test-Path $csv) {
        Invocar 'uv' @('run', 'viz/plotar_experimentos.py', $csv)
    }
    else {
        Write-Warning "Sem $csv; pulando as figuras dos experimentos."
    }

    # O relatório aponta para docs/figuras/, porque saida/ não vai para o git.
    if ($Exemplo) {
        Write-Host "Modo -Exemplo: nada é copiado para docs/figuras/."
    }
    else {
        Etapa 'Copiando as figuras para docs/figuras/'
        $destino = Join-Path 'docs' 'figuras'
        $copias = @($pastas | ForEach-Object { @{ de = (Join-Path $_ 'figuras'); para = (Split-Path $_ -Leaf) } })
        $copias += @{ de = (Join-Path $pastaSaida 'figuras'); para = 'experimentos' }
        foreach ($c in $copias) {
            if (Test-Path $c.de) {
                $alvo = Join-Path $destino $c.para
                New-Item -ItemType Directory -Force $alvo | Out-Null
                Copy-Item (Join-Path $c.de '*') $alvo -Force
                Write-Host "  $($c.de) -> $alvo"
            }
        }
    }

    Etapa ("Pronto em {0:N0} s. Figuras em {1}/*/figuras/ e {1}/figuras/" -f
           $relogio.Elapsed.TotalSeconds, $pastaSaida)
}
finally {
    Pop-Location
}
