# OpenBioUnlock

OpenBioUnlock is a local-first biometric approval bridge for workstation authentication. A phone authenticates the user locally and signs a short-lived daemon challenge. The workstation verifies the signature; passwords are never sent through the phone, BLE, TCP, or PAM module.

## Repository layout

- `daemon/`: Rust service, OS keyring integration, TCP/Unix/named-pipe IPC, and signature verification.
- `src/`: Qt desktop controller with the system tray lifecycle, TCP/UDP discovery server, OpenSSL cryptography, OS credential vault wrapper, dashboard, and QR pairing dialog.
- `windows-cp/`: Windows Credential Provider COM adapter and installer.
- `linux-pam/`: PAM adapter for sudo/display managers and installer.
- `mobile/`: Flutter client with biometric signing and BLE proximity discovery.
- `proto/`: versioned cross-platform authentication schema.
- `.github/workflows/ci.yml`: hosted-runner build and test pipeline.

## CI and artifacts

## Desktop build

The native desktop controller requires Qt 6.4 or newer, OpenSSL 3, CMake 3.21 or newer, and the Nayuki QR Code Generator dependency downloaded by CMake. Linux builds also require `libsecret-1` development headers and an active Secret Service provider. Configure and build it with:

```bash
cmake -S . -B build
cmake --build build --config Release
```

The desktop listener uses TCP and UDP port `43295` by default. TCP is bound to loopback; UDP discovery uses the local broadcast address. Pairing payloads are exchanged out of band through the QR ceremony.

Push to `main` or `master`, or open a pull request targeting either branch. GitHub Actions runs Rust tests and a release daemon build, PAM compilation, Flutter analysis/tests and Android APK compilation, and Windows MSVC compilation of the Credential Provider DLL.

Artifacts are named `openbiounlock-linux`, `openbiounlock-android`, and `openbiounlock-windows`. The workflow does not sign release binaries or install them on a workstation. Future signed releases require protected secrets such as `WINDOWS_CERTIFICATE_BASE64`, `WINDOWS_CERTIFICATE_PASSWORD`, and `ANDROID_KEYSTORE_BASE64`; never commit those values.

## Linux installation

Requirements: supported Linux, Rust stable, a C compiler, PAM development headers, and an active Secret Service/keyring provider.

```bash
sudo apt-get install build-essential libpam0g-dev pkg-config
./linux-pam/install.sh
```

The installer builds the daemon and PAM module, installs the daemon at `/usr/local/bin/openbiounlock-daemon`, installs the module at `/lib/security/pam_openbiounlock.so`, and configures `/var/run/openbiounlock.sock`. Review `/etc/pam.d/sudo` before enabling it. Keep a second root session open while testing PAM changes.

Start the daemon under a service manager with `RUST_LOG=info` or `RUST_LOG=debug`. The daemon should run as a dedicated service account, and the socket should be owned by the intended authentication group.

## Windows installation

Requirements: Windows SDK, Visual Studio C++ build tools, Rust stable, administrator rights, and a suitable code-signing certificate.

Build the daemon and DLL in CI or locally with MSVC. Copy the signed artifacts into `C:\Program Files\OpenBioUnlock`, then run `windows-cp/install.ps1` from elevated PowerShell. The script registers the COM CLSID, Credential Provider, and automatic daemon service. Test in a disposable VM first because Credential Providers load inside `LogonUI.exe`.

## Pairing and BLE

1. Start the daemon and open the mobile app.
2. Select **Pair a PC** and scan the PC QR payload.
3. Confirm the displayed host, port, PC identity, X25519 public key, and one-time pairing code out of band.
4. The app stores its signing seed in platform secure storage and sends only the Ed25519 public key during pairing.
5. Enable proximity scanning. The app filters for the OpenBioUnlock BLE service UUID and applies near/far RSSI hysteresis.
6. Proximity is only a discovery signal. Unlock still requires a fresh daemon challenge, biometric approval, and valid Ed25519 signature.

Enable Bluetooth permissions in Android/iOS settings. RSSI is affected by walls, body position, and interference; use it for convenience and walkaway locking, never as sole authorization.

## Configuration and environment

- `RUST_LOG`: tracing filter, for example `info` or `debug`.
- `OPENBIOUNLOCK_SOCKET`: deployment-specific socket override for service wrappers and PAM arguments.
- `OPENBIOUNLOCK_PAIRING_TTL`: deployment policy for one-time pairing-code expiry.

Reference endpoints are TCP loopback `127.0.0.1:45821`, Unix socket `/var/run/openbiounlock.sock`, and Windows named pipe `\\.\pipe\OpenBioUnlockPipe`. Do not expose TCP beyond loopback without authenticated encryption and firewall policy.

## Troubleshooting

**No Linux challenge:** check the daemon, socket existence, permissions, and `RUST_LOG=debug` service logs.

**PAM rejects every request:** confirm pairing, keyring contents, a challenge age under 30 seconds, and a working fallback authentication rule.

**BLE does not show the PC:** enable permissions, verify Bluetooth is on, confirm the custom service UUID is advertised, and move closer.

**Windows provider is absent:** verify DLL signing and architecture, confirm `InprocServer32` points to the installed DLL, restart LogonUI or reboot, and inspect Event Viewer.

**CI cannot build Flutter:** the workflow generates the Android runner before dependency resolution. Review the Flutter version and package compatibility in the failed job; native BLE and biometric behavior still requires device testing.

## Security boundaries

The design assumes trusted host and phone operating systems and an in-person pairing ceremony. It does not protect against a compromised kernel, rooted phone, stolen unlocked phone, or malware with access to the authenticated user session. Review and sign all native artifacts before deployment.
