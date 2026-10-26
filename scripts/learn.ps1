<#
    learn — compila, testa e corre os exercícios.

    Uso (a partir da raiz do repositório):
        ./learn check 03        compila e corre os testes do exercício 03
        ./learn check all       todos os exercícios
        ./learn run 03          compila e abre o standalone do exercício 03
        ./learn web             arranca a web app (http://localhost:5173)
        ./learn list            currículo e estado
        ./learn smoke 03|all    abre o standalone ~3 s e verifica que sai som
        ./learn help

    Opções: -v (mostra o output completo do Catch2), --release (compila em Release)

    Compatível com Windows PowerShell 5.1 (o que vem com o Windows).
#>

# "Continue" e não "Stop": no PowerShell 5.1, o stderr de programas nativos (cmake, cl) redirecionado
# com 2>&1 vira ErrorRecord e com "Stop" interromperia o script. Os erros são tratados à mão.
$ErrorActionPreference = "Continue"
Set-StrictMode -Version 2.0

try { [Console]::OutputEncoding = New-Object System.Text.UTF8Encoding $false } catch {}

$RepoRoot      = Split-Path -Parent $PSScriptRoot
$BuildDir      = Join-Path $RepoRoot "build"
$LogDir        = Join-Path $BuildDir "learn-logs"
$ProgressFile  = Join-Path $RepoRoot "progress.json"
$HashesFile    = Join-Path $RepoRoot "exercises\starter-hashes.json"
$StudentFiles  = @("Processor.h", "Processor.cpp")

#===================================================================================================
# Output

function Write-Title ([string] $text) { Write-Host ""; Write-Host $text -ForegroundColor Cyan }
function Write-Note  ([string] $text) { Write-Host $text -ForegroundColor DarkGray }
function Write-Good  ([string] $text) { Write-Host $text -ForegroundColor Green }
function Write-Bad   ([string] $text) { Write-Host $text -ForegroundColor Red }
function Write-Warn  ([string] $text) { Write-Host $text -ForegroundColor Yellow }

function Fail ([string] $text)
{
    Write-Bad $text
    exit 1
}

#===================================================================================================
# Ambiente do Visual Studio (cl.exe, ninja)

function Import-VsEnvironment
{
    if ((Get-Command cl.exe -ErrorAction SilentlyContinue) -and (Get-Command ninja.exe -ErrorAction SilentlyContinue))
    {
        return
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"

    if (-not (Test-Path $vswhere))
    {
        Fail "Não encontrei o Visual Studio (vswhere.exe). Instala o Visual Studio 2022 com 'Desktop development with C++' (ver README)."
    }

    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath

    if (-not $installPath)
    {
        Fail "O Visual Studio não tem o compilador C++ (MSVC). Instala o workload 'Desktop development with C++'."
    }

    $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
    Write-Note "A ativar o ambiente do Visual Studio ($installPath)"

    $lines = & cmd.exe /c "`"$vcvars`" >nul 2>&1 && set"

    foreach ($line in $lines)
    {
        $index = $line.IndexOf("=")

        if ($index -gt 0)
        {
            [Environment]::SetEnvironmentVariable($line.Substring(0, $index), $line.Substring($index + 1), "Process")
        }
    }

    if (-not (Get-Command ninja.exe -ErrorAction SilentlyContinue))
    {
        Fail "Não encontrei o ninja. Instala o componente 'C++ CMake tools for Windows' do Visual Studio."
    }
}

#===================================================================================================
# CMake

function Invoke-Configure ([switch] $Force)
{
    if ((Test-Path (Join-Path $BuildDir "CMakeCache.txt")) -and -not $Force)
    {
        return
    }

    Write-Title "A configurar o projeto (cmake --preset default)"
    Write-Note  "Na primeira vez descarrega o JUCE 9.0.4 e o Catch2 e pode demorar vários minutos."

    Push-Location $RepoRoot
    try
    {
        & cmake --preset default 2>&1 | ForEach-Object { Write-Note "  $_" }
        if ($LASTEXITCODE -ne 0) { Fail "A configuração do CMake falhou." }
    }
    finally { Pop-Location }
}

# Compila um alvo. Devolve $true/$false. Guarda o log completo em build/learn-logs/<alvo>.log
function Invoke-Build ([string[]] $Targets, [string] $Config)
{
    New-Item -ItemType Directory -Force $LogDir | Out-Null
    $log = Join-Path $LogDir ("build-" + ($Targets -join "_") + ".log")
    $output = New-Object System.Collections.Generic.List[string]

    $targetArgs = @()
    foreach ($t in $Targets) { $targetArgs += @("--target", $t) }

    & cmake --build $BuildDir --config $Config @targetArgs 2>&1 | ForEach-Object {
        $line = "$_"
        $output.Add($line)

        if ($line -match '^\[(\d+)/(\d+)\]')
        {
            $done = [int] $Matches[1]; $total = [int] $Matches[2]
            Write-Progress -Activity "A compilar ($Config)" -Status $line.Substring(0, [Math]::Min(100, $line.Length)) -PercentComplete ([int] (100 * $done / [Math]::Max(1, $total)))
        }
    }
    $exitCode = $LASTEXITCODE
    Write-Progress -Activity "A compilar" -Completed

    # Um exercício novo (pasta nova) ainda não é um alvo conhecido: reconfigura e tenta outra vez.
    if ($exitCode -ne 0 -and -not $script:Reconfigured -and ($output -match 'unknown target'))
    {
        $script:Reconfigured = $true
        Write-Note "Alvo desconhecido: a reconfigurar o CMake (há exercícios novos?)"
        Invoke-Configure -Force
        return (Invoke-Build $Targets $Config)
    }

    [IO.File]::WriteAllLines($log, $output)

    if ($exitCode -ne 0)
    {
        Show-BuildErrors $output
        Write-Note "Log completo: $log"
        return $false
    }

    # Avisos nos ficheiros dos exercícios (não nos do JUCE)
    $warnings = $output | Where-Object { $_ -match 'warning C\d+' -and $_ -match '\\exercises\\' } | Select-Object -Unique

    if ($warnings)
    {
        Write-Warn "Avisos do compilador no teu código:"
        $warnings | Select-Object -First 10 | ForEach-Object { Write-Warn ("  " + (Format-CompilerLine $_)) }
    }

    return $true
}

function Format-CompilerLine ([string] $line)
{
    return $line.Replace($RepoRoot + "\", "").Trim()
}

function Show-BuildErrors ($output)
{
    Write-Bad "A compilação falhou:"
    $errors = $output | Where-Object { $_ -match '(error C\d+|error LNK\d+|fatal error|CMake Error)' } | Select-Object -Unique

    if (-not $errors)
    {
        $output | Select-Object -Last 25 | ForEach-Object { Write-Host "  $_" }
        return
    }

    $errors | Select-Object -First 15 | ForEach-Object { Write-Bad ("  " + (Format-CompilerLine $_)) }

    if (@($errors).Count -gt 15)
    {
        Write-Note ("  ... e mais " + (@($errors).Count - 15) + " erros")
    }
}

#===================================================================================================
# Currículo

function Get-Curriculum
{
    $json = Get-Content -Raw -Encoding UTF8 (Join-Path $RepoRoot "curriculum.json") | ConvertFrom-Json
    $list = @()

    foreach ($module in $json.modules)
    {
        foreach ($exercise in $module.exercises)
        {
            $exercise | Add-Member -NotePropertyName module -NotePropertyValue $module.id -Force
            $exercise | Add-Member -NotePropertyName moduleTitle -NotePropertyValue $module.title -Force
            $list += $exercise
        }
    }

    return $list
}

function Get-OptionalProperty ($object, [string] $name, $default)
{
    if ($object.PSObject.Properties.Name -contains $name) { return $object.$name }
    return $default
}

function Resolve-Exercises ([string] $selector)
{
    $all = Get-Curriculum
    $available = $all | Where-Object { Test-Path (Join-Path $RepoRoot ("exercises\" + $_.folder + "\CMakeLists.txt")) }

    if ($selector -eq "all")
    {
        return @($available)
    }

    if ($selector -notmatch '^\d{1,2}$')
    {
        Fail "Exercício inválido: '$selector'. Usa um número (ex.: 03) ou 'all'."
    }

    $id = "{0:D2}" -f [int] $selector
    $exercise = $all | Where-Object { $_.id -eq $id }

    if (-not $exercise) { Fail "Não existe o exercício $id no currículo." }

    if (-not (Test-Path (Join-Path $RepoRoot ("exercises\" + $exercise.folder + "\CMakeLists.txt"))))
    {
        Fail "O exercício $id ($($exercise.folder)) ainda não existe neste branch."
    }

    return @($exercise)
}

function Get-ExerciseConfig ($exercise)
{
    if ($script:ForceRelease) { return "Release" }
    return (Get-OptionalProperty $exercise "config" "Debug")
}

#===================================================================================================
# Estado (progress.json)

function Read-JsonFile ([string] $path)
{
    if (-not (Test-Path $path)) { return $null }
    return (Get-Content -Raw -Encoding UTF8 $path | ConvertFrom-Json)
}

# SHA-256 do conteúdo sem CR (0x0D): o mesmo hash com finais de linha LF ou CRLF (core.autocrlf).
function Get-NormalizedHash ([string] $path)
{
    $bytes = [IO.File]::ReadAllBytes($path)
    $normalized = New-Object System.Collections.Generic.List[byte] ($bytes.Length)
    foreach ($b in $bytes) { if ($b -ne 13) { $normalized.Add($b) } }

    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($normalized.ToArray()))).Replace("-", "") }
    finally { $sha.Dispose() }
}

function Test-ExerciseModified ($exercise)
{
    $hashes = Read-JsonFile $HashesFile
    if (-not $hashes) { return $false }
    if (-not ($hashes.PSObject.Properties.Name -contains $exercise.id)) { return $false }

    $starter = $hashes.($exercise.id)

    foreach ($file in $starter.PSObject.Properties.Name)
    {
        $path = Join-Path $RepoRoot ("exercises\" + $exercise.folder + "\" + $file)
        if (-not (Test-Path $path)) { return $true }
        if ((Get-NormalizedHash $path) -ne $starter.$file) { return $true }
    }

    return $false
}

function Update-Progress ($exercise, [string] $status, [int] $passed, [int] $total, $failures)
{
    $progress = Read-JsonFile $ProgressFile
    $entries = [ordered] @{}

    if ($progress -and ($progress.PSObject.Properties.Name -contains "exercises"))
    {
        foreach ($p in $progress.exercises.PSObject.Properties)
        {
            $entries[$p.Name] = $p.Value
        }
    }

    $entries[$exercise.id] = [ordered] @{
        status   = $status
        passed   = $passed
        total    = $total
        failures = @($failures | Select-Object -First 20)
        lastRun  = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    }

    $branch = ""
    try { $branch = (& git -C $RepoRoot rev-parse --abbrev-ref HEAD 2>$null) } catch {}

    $document = [ordered] @{
        generatedAt = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
        branch      = "$branch"
        exercises   = $entries
    }

    $json = $document | ConvertTo-Json -Depth 6
    [IO.File]::WriteAllText($ProgressFile, $json, (New-Object System.Text.UTF8Encoding $false))
}

#===================================================================================================
# Testes

function Get-TestExecutable ($exercise, [string] $config)
{
    return Join-Path $BuildDir ("exercises\" + $exercise.folder + "\" + $config + "\ex" + $exercise.id + "_tests.exe")
}

# Corre o executável Catch2 com dois reporters: JUnit (para nós) e consola (para ti / para o /tutor).
function Invoke-Tests ($exercise, [string] $config)
{
    $exe = Get-TestExecutable $exercise $config
    $junit = Join-Path $LogDir ("ex" + $exercise.id + "-junit.xml")
    $consoleLog = Join-Path $LogDir ("ex" + $exercise.id + "-tests.txt")
    Remove-Item $junit, $consoleLog -ErrorAction SilentlyContinue

    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $exe
    $info.WorkingDirectory = $LogDir
    # Caminhos relativos: evita problemas com o espaço em "Learn JUCE"
    $info.Arguments = "--order decl --reporter junit::out=ex$($exercise.id)-junit.xml --reporter console::out=ex$($exercise.id)-tests.txt::colour-mode=none"
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::Start($info)
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    if (-not $process.WaitForExit(180000))
    {
        $process.Kill()
        return @{ timedOut = $true; cases = @(); exitCode = -1; consoleLog = $consoleLog; stderr = "" }
    }

    $stderr = $stderrTask.Result + $stdoutTask.Result

    $cases = [ordered] @{}

    $xml = $null

    if (Test-Path $junit)
    {
        # Se o processo rebentou a meio, o XML pode estar incompleto
        try { $xml = [xml] (Get-Content -Raw -Encoding UTF8 $junit) } catch { $xml = $null }
    }

    if ($xml)
    {
        foreach ($case in $xml.SelectNodes("//testcase"))
        {
            # Catch2 junta as secções ao nome com '/'; agrupamos pelo TEST_CASE
            $name = ($case.GetAttribute("name") -split '/')[0]

            if (-not $cases.Contains($name))
            {
                $cases[$name] = @{ name = $name; failed = $false; messages = New-Object System.Collections.Generic.List[string] }
            }

            foreach ($failure in @($case.SelectNodes("failure")) + @($case.SelectNodes("error")))
            {
                $cases[$name].failed = $true
                $cases[$name].messages.Add($failure.InnerText.Trim())
            }
        }
    }

    return @{ timedOut = $false; cases = @($cases.Values); exitCode = $process.ExitCode; consoleLog = $consoleLog; stderr = $stderr }
}

function Show-TestResults ($result, [switch] $Detailed)
{
    foreach ($case in $result.cases)
    {
        if ($case.failed)
        {
            Write-Bad ("  ✘ " + $case.name)

            $message = $case.messages | Select-Object -First 1
            $lines = ($message -split "`n") | ForEach-Object { $_.TrimEnd() } | Where-Object { $_ -ne "" }

            foreach ($line in ($lines | Select-Object -First 8))
            {
                Write-Host ("      " + (Format-CompilerLine $line)) -ForegroundColor DarkYellow
            }
        }
        else
        {
            Write-Good ("  ✔ " + $case.name)
        }
    }

    if ($Detailed -and (Test-Path $result.consoleLog))
    {
        Write-Title "Output completo do Catch2"
        Get-Content $result.consoleLog | ForEach-Object { Write-Host "  $_" }
    }
}

function Invoke-Check ($exercise, [switch] $Detailed, [switch] $Quiet)
{
    $config = Get-ExerciseConfig $exercise
    $title = "Exercício $($exercise.id) · $($exercise.title)"

    if (-not $Quiet) { Write-Title "$title  [$config]" }

    $modified = Test-ExerciseModified $exercise
    $built = Invoke-Build @("ex$($exercise.id)_tests") $config

    if (-not $built)
    {
        $status = "not_started"
        if ($modified) { $status = "in_progress" }
        Update-Progress $exercise $status 0 0 @("(não compila)")
        return @{ id = $exercise.id; title = $exercise.title; status = $status; passed = 0; total = 0; build = $false }
    }

    $result = Invoke-Tests $exercise $config

    if ($result.timedOut)
    {
        Write-Bad "  Os testes não terminaram em 3 minutos (ciclo infinito?)."
        Update-Progress $exercise "in_progress" 0 0 @("(timeout)")
        return @{ id = $exercise.id; title = $exercise.title; status = "in_progress"; passed = 0; total = 0; build = $true }
    }

    $total = @($result.cases).Count
    $passed = @($result.cases | Where-Object { -not $_.failed }).Count
    $failures = @($result.cases | Where-Object { $_.failed } | ForEach-Object { $_.name })

    if ($total -eq 0)
    {
        Write-Bad "  Os testes terminaram antes do fim (código $($result.exitCode)): provavelmente um crash,"
        Write-Bad "  por exemplo um acesso fora de um buffer/array. Últimas linhas:"

        $tail = @()
        if (Test-Path $result.consoleLog) { $tail += Get-Content $result.consoleLog | Select-Object -Last 8 }
        if ($result.stderr) { $tail += ($result.stderr -split "`n") | Where-Object { $_.Trim() -ne "" } | Select-Object -Last 8 }
        $tail | ForEach-Object { Write-Host ("      " + (Format-CompilerLine $_)) -ForegroundColor DarkYellow }

        $failures = @("(crash: os testes terminaram antes do fim)")
    }

    if (-not $Quiet) { Show-TestResults $result -Detailed:$Detailed }

    if ($total -gt 0 -and $passed -eq $total -and $result.exitCode -eq 0)
    {
        $status = "done"
        if (-not $Quiet) { Write-Good "  Todos os testes passam ($passed/$total). Bom trabalho!" }
    }
    else
    {
        $status = "not_started"
        if ($modified) { $status = "in_progress" }

        if (-not $Quiet)
        {
            Write-Warn "  $passed/$total testes passam."
            Write-Note "  Precisas de ajuda? No Claude Code: /tutor $($exercise.id)    Output completo: ./learn check $($exercise.id) -v"
        }
    }

    Update-Progress $exercise $status $passed $total $failures
    return @{ id = $exercise.id; title = $exercise.title; status = $status; passed = $passed; total = $total; build = $true }
}

#===================================================================================================
# Comandos

function Command-Check ([string] $selector, [switch] $Detailed)
{
    if (-not $selector) { Fail "Uso: ./learn check <NN|all>" }

    Import-VsEnvironment
    Invoke-Configure
    $exercises = Resolve-Exercises $selector

    if ($selector -ne "all")
    {
        $r = Invoke-Check $exercises[0] -Detailed:$Detailed
        if ($r.status -ne "done") { exit 1 }
        return
    }

    $results = @()

    foreach ($exercise in $exercises)
    {
        Write-Host ("A verificar {0} · {1} ..." -f $exercise.id, $exercise.title) -ForegroundColor DarkGray
        $results += Invoke-Check $exercise -Quiet
    }

    Write-Title "Resumo"

    foreach ($r in $results)
    {
        $label = "{0}  {1,-40} {2}/{3}" -f $r.id, $r.title, $r.passed, $r.total

        if (-not $r.build)              { Write-Bad  ("  ✘ $label  (não compila)") }
        elseif ($r.status -eq "done")   { Write-Good ("  ✔ $label") }
        elseif ($r.status -eq "in_progress") { Write-Warn ("  … $label  (em curso)") }
        else                            { Write-Note ("  · $label") }
    }

    $done = @($results | Where-Object { $_.status -eq "done" }).Count
    Write-Host ""
    Write-Host "  $done de $(@($results).Count) exercícios concluídos." -ForegroundColor Cyan
}

function Find-AppExecutable ($exercise, [string] $config)
{
    $folder = Join-Path $BuildDir ("exercises\" + $exercise.folder)
    $candidates = Get-ChildItem -Path $folder -Recurse -Filter *.exe -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '_artefacts\\' + $config + '\\' }

    $standalone = $candidates | Where-Object { $_.FullName -match '\\Standalone\\' }
    if ($standalone) { return ($standalone | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName }

    return ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}

function Get-RunTargets ($exercise)
{
    return @(Get-OptionalProperty $exercise "runTargets" @("ex$($exercise.id)_app"))
}

function Command-Run ([string] $selector)
{
    if (-not $selector -or $selector -eq "all") { Fail "Uso: ./learn run <NN>" }

    Import-VsEnvironment
    Invoke-Configure
    $exercise = (Resolve-Exercises $selector)[0]
    $config = Get-ExerciseConfig $exercise

    Write-Title "A compilar o standalone do exercício $($exercise.id) [$config]"
    if (-not (Invoke-Build (Get-RunTargets $exercise) $config)) { exit 1 }

    $exe = Find-AppExecutable $exercise $config
    if (-not $exe) { Fail "Não encontrei o executável do standalone." }

    Write-Good "A abrir: $exe"
    Write-Note "Dica: começa com o volume baixo. O standalone tem um limitador de proteção, mas os ouvidos agradecem."
    Start-Process -FilePath $exe
}

function Command-Smoke ([string] $selector)
{
    if (-not $selector) { Fail "Uso: ./learn smoke <NN|all>" }

    Import-VsEnvironment
    Invoke-Configure
    $failures = 0

    foreach ($exercise in (Resolve-Exercises $selector))
    {
        $config = Get-ExerciseConfig $exercise
        Write-Host ("Standalone {0} · {1} ..." -f $exercise.id, $exercise.title) -ForegroundColor DarkGray

        if (-not (Invoke-Build (Get-RunTargets $exercise) $config)) { $failures++; continue }

        $exe = Find-AppExecutable $exercise $config
        $resultFile = Join-Path $LogDir ("smoke-ex" + $exercise.id + ".json")
        Remove-Item $resultFile -ErrorAction SilentlyContinue

        $process = Start-Process -FilePath $exe -ArgumentList "--smoke-test=`"$resultFile`"" -PassThru
        if (-not $process.WaitForExit(30000)) { $process.Kill() }

        $result = Read-JsonFile $resultFile

        if ($result -and $result.ok)
        {
            Write-Good ("  ✔ abre e produz som ({0} Hz, pico {1:N3})" -f $result.sampleRate, [double] $result.peak)
        }
        else
        {
            $failures++
            Write-Bad "  ✘ sem som, NaN ou não arrancou ($resultFile)"
        }
    }

    if ($failures -gt 0) { exit 1 }
}

function Command-List
{
    $progress = Read-JsonFile $ProgressFile
    $currentModule = 0

    foreach ($exercise in (Get-Curriculum))
    {
        if ($exercise.module -ne $currentModule)
        {
            $currentModule = $exercise.module
            Write-Title ("Módulo {0} — {1}" -f $exercise.module, $exercise.moduleTitle)
        }

        $exists = Test-Path (Join-Path $RepoRoot ("exercises\" + $exercise.folder + "\CMakeLists.txt"))
        $status = "not_started"
        $score = ""

        if ($progress -and ($progress.exercises.PSObject.Properties.Name -contains $exercise.id))
        {
            $entry = $progress.exercises.($exercise.id)
            $status = $entry.status
            $score = "{0}/{1}" -f $entry.passed, $entry.total
        }

        $label = "{0}  {1,-42} {2,4} min  {3}" -f $exercise.id, $exercise.title, $exercise.minutes, $score

        if (-not $exists)                  { Write-Note "  - $label (ainda não existe)" }
        elseif ($status -eq "done")        { Write-Good "  ✔ $label" }
        elseif ($status -eq "in_progress") { Write-Warn "  … $label" }
        else                               { Write-Host "  · $label" }
    }

    Write-Host ""
}

function Command-Web
{
    $webDir = Join-Path $RepoRoot "web"

    if (-not (Get-Command npm -ErrorAction SilentlyContinue))
    {
        Fail "Não encontrei o npm. Instala o Node.js LTS (ver README)."
    }

    Push-Location $webDir
    try
    {
        if (-not (Test-Path (Join-Path $webDir "node_modules")))
        {
            Write-Title "A instalar as dependências da web app (npm install)"
            & npm install
            if ($LASTEXITCODE -ne 0) { Fail "npm install falhou." }
        }

        Write-Title "Web app em http://localhost:5173  (Ctrl+C para parar)"
        & npm run dev -- --open
    }
    finally { Pop-Location }
}

# Para quem mantém o curso: regista o hash dos ficheiros iniciais (com TODO) de cada exercício.
function Command-UpdateStarterHashes
{
    $hashes = [ordered] @{}

    foreach ($exercise in (Get-Curriculum))
    {
        $folder = Join-Path $RepoRoot ("exercises\" + $exercise.folder)
        if (-not (Test-Path $folder)) { continue }

        $files = [ordered] @{}

        foreach ($file in $StudentFiles)
        {
            $path = Join-Path $folder $file
            if (Test-Path $path) { $files[$file] = Get-NormalizedHash $path }
        }

        $hashes[$exercise.id] = $files
    }

    [IO.File]::WriteAllText($HashesFile, ($hashes | ConvertTo-Json -Depth 4), (New-Object System.Text.UTF8Encoding $false))
    Write-Good "Atualizado: $HashesFile"
}

function Command-Help
{
    Write-Host @"

learn — plataforma para aprender JUCE e DSP

  ./learn check NN       compila e corre os testes do exercício NN (ex.: ./learn check 03)
  ./learn check all      o mesmo para todos os exercícios
  ./learn run NN         compila e abre o standalone do exercício
  ./learn web            arranca a web app de teoria em http://localhost:5173
  ./learn list           currículo e estado de cada exercício
  ./learn smoke NN|all   abre o standalone ~3 s e verifica que sai som

  Opções:  -v          mostra o output completo do Catch2
           --release   compila em Release (por omissão: Debug, exceto o módulo 9)

"@
}

#===================================================================================================
# Main

$positional = @()
$detailed = $false
$script:ForceRelease = $false
$script:Reconfigured = $false

foreach ($a in $args)
{
    switch -regex ($a)
    {
        '^(-v|--verbose|-verbose)$' { $detailed = $true }
        '^(--release|-release)$'    { $script:ForceRelease = $true }
        default                     { $positional += $a }
    }
}

$command = "help"
$target = ""
if ($positional.Count -ge 1) { $command = $positional[0].ToLower() }
if ($positional.Count -ge 2) { $target = $positional[1].ToLower() }

switch ($command)
{
    "check"                 { Command-Check $target -Detailed:$detailed }
    "run"                   { Command-Run $target }
    "smoke"                 { Command-Smoke $target }
    "list"                  { Command-List }
    "web"                   { Command-Web }
    "configure"             { Import-VsEnvironment; Invoke-Configure -Force }
    "update-starter-hashes" { Command-UpdateStarterHashes }
    default                 { Command-Help }
}
