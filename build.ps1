# build.ps1 - V3 (Change Directory Method)
$ErrorActionPreference = "Stop"

# --- Configuration ---
$projectRoot = $PSScriptRoot
$compilerDir = "C:\msys64\mingw64\bin"
$compilerExe = Join-Path $compilerDir "g++.exe"
$includePath = "C:\msys64\mingw64\include"
$libPath = "C:\msys64\mingw64\lib"
$raylibLibs = "-lraylib", "-lgdi32", "-lwinmm"
$outputExe = Join-Path $projectRoot "build\EchoesOfTheRift.exe"

$sourceFiles = Get-ChildItem (Join-Path $projectRoot "src\*.cpp") | ForEach-Object { $_.FullName }
$objectFiles = $sourceFiles | ForEach-Object { Join-Path $projectRoot ($_.Replace($projectRoot, "").Replace("src\", "build\obj\").Replace(".cpp", ".obj")) }

# --- Build Process ---
Write-Host "Project Root: $projectRoot"
Write-Host "Compiler Directory: $compilerDir"

# Create build directories
$buildDir = Join-Path $projectRoot "build"
$objDir = Join-Path $projectRoot "build\obj"
if (-not (Test-Path $buildDir)) { New-Item -ItemType Directory -Force $buildDir }
if (-not (Test-Path $objDir)) { New-Item -ItemType Directory -Force $objDir }

# Clean previous build artifacts
Remove-Item $outputExe -ErrorAction SilentlyContinue
Remove-Item (Join-Path $objDir "*.obj") -ErrorAction SilentlyContinue

# Change to the compiler's directory
Push-Location $compilerDir

try {
    # 1. Compile each source file
    Write-Host "--- Starting Compilation (from $PWD) ---" -ForegroundColor Green
    foreach ($source in $sourceFiles) {
        $object = $source.Replace((Join-Path $projectRoot "src\"), $objDir + "\").Replace(".cpp", ".obj")
        Write-Host "Compiling `"$source`"..."
        & $compilerExe -c "$source" -o "$object" -I "$includePath" -std=c++17
        if ($LASTEXITCODE -ne 0) { throw "Compilation of $source failed!" }
    }
    Write-Host "--- Compilation Successful ---" -ForegroundColor Green

    # 2. Link all object files
    Write-Host "--- Starting Linking ---" -ForegroundColor Green
    $objectListString = $objectFiles -join '" "'
    & $compilerExe $objectFiles -o "$outputExe" -L "$libPath" $raylibLibs
    if ($LASTEXITCODE -ne 0) { throw "Linking failed!" }
    Write-Host "--- Linking Successful ---" -ForegroundColor Green

}
catch {
    Write-Host "## BUILD FAILED: $($_.Exception.Message) ##" -ForegroundColor Red
}
finally {
    # Return to the original directory
    Pop-Location
}

# 3. Final Verification
Write-Host "--- Verifying Executable ---" -ForegroundColor Green
if (-not (Test-Path $outputExe)) {
    Write-Host "## VERIFICATION FAILED: Executable not found! ##" -ForegroundColor Red
    exit 1
}

Write-Host "## BUILD SUCCEEDED! Executable is at `"$outputExe`" ##" -ForegroundColor Cyan

# 4. Copy Assets and DLLs to Build Directory
Write-Host "--- Copying Assets and DLLs ---" -ForegroundColor Green
$assetDir = Join-Path $projectRoot "assets"
$buildDir = Join-Path $projectRoot "build"
if (Test-Path $assetDir) {
    $buildAssetDir = Join-Path $buildDir "assets"
    if (-not (Test-Path $buildAssetDir)) { New-Item -ItemType Directory -Force $buildAssetDir | Out-Null }
    Copy-Item -Path "$assetDir\*" -Destination $buildAssetDir -Recurse -Force
    Write-Host "Assets copied."
}
# Copy required Raylib DLLs
Copy-Item -Path (Join-Path $compilerDir "libraylib.dll") -Destination $buildDir -Force
Copy-Item -Path (Join-Path $compilerDir "libgcc_s_seh-1.dll") -Destination $buildDir -Force
Copy-Item -Path (Join-Path $compilerDir "libstdc++-6.dll") -Destination $buildDir -Force
Copy-Item -Path (Join-Path $compilerDir "libwinpthread-1.dll") -Destination $buildDir -Force
Copy-Item -Path (Join-Path "C:\msys64\mingw64\bin" "glfw3.dll") -Destination $buildDir -Force # GLFW is often in bin

Write-Host "DLLs copied."
Write-Host "--- Asset Copy Complete ---" -ForegroundColor Green
