$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$required = @('cargo', 'flutter')
$missing = @($required | Where-Object { -not (Get-Command $_ -ErrorAction SilentlyContinue) })
if ($missing.Count -gt 0) {
    throw "Missing required toolchain(s): $($missing -join ', '). Install Rust/Cargo and Flutter, then rerun build_all.ps1. The script will not install unsigned toolchains or modify system policy automatically."
}
if (-not (Get-Command cl -ErrorAction SilentlyContinue) -and -not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw 'Missing C++ compiler: install Visual Studio Build Tools with the Desktop C++ workload.'
}
Push-Location $root
try {
    cargo test --manifest-path daemon\Cargo.toml
    cargo build --manifest-path daemon\Cargo.toml --release
    flutter pub get --directory mobile
    flutter analyze --directory mobile
    flutter test --directory mobile
    Write-Host 'OpenBioUnlock build and test checks passed.'
} finally { Pop-Location }
