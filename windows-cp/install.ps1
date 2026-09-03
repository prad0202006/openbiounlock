$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
cargo build --manifest-path (Join-Path $Root 'daemon\Cargo.toml') --release
$service = Join-Path $Root 'daemon\target\release\openbiounlock-daemon.exe'
if (-not (Test-Path $service)) { throw 'Daemon build did not produce an executable.' }
New-Item -ItemType Directory -Force 'C:\Program Files\OpenBioUnlock' | Out-Null
Copy-Item $service 'C:\Program Files\OpenBioUnlock\openbiounlock-daemon.exe' -Force
if (Test-Path (Join-Path $Root 'windows-cp\out\OpenBioUnlockCredentialProvider.dll')) {
	Copy-Item (Join-Path $Root 'windows-cp\out\OpenBioUnlockCredentialProvider.dll') 'C:\Program Files\OpenBioUnlock\OpenBioUnlockCredentialProvider.dll' -Force
}
sc.exe create OpenBioUnlockDaemon binPath= '"C:\Program Files\OpenBioUnlock\openbiounlock-daemon.exe"' start= auto | Out-Host
New-Item -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{D1AA6D25-6F49-4A52-A2CF-6F22D9D19001}' -Force | Out-Null
Set-ItemProperty -Path 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{D1AA6D25-6F49-4A52-A2CF-6F22D9D19001}' -Name '(Default)' -Value 'OpenBioUnlock'
$clsid = '{D1AA6D25-6F49-4A52-A2CF-6F22D9D19001}'
$dll = 'C:\Program Files\OpenBioUnlock\OpenBioUnlockCredentialProvider.dll'
New-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Name '(Default)' -Value 'OpenBioUnlock Credential Provider'
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name '(Default)' -Value $dll
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name 'ThreadingModel' -Value 'Apartment'
New-Item -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\$clsid" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\$clsid" -Name '(Default)' -Value 'OpenBioUnlock'
Write-Host 'Daemon installed. Build and register the signed Credential Provider DLL before enabling the provider.'
