#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
missing=()
for tool in cargo flutter cmake; do
  command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done
if command -v g++ >/dev/null 2>&1; then :; elif command -v cc >/dev/null 2>&1; then :; else missing+=("gcc/clang"); fi
if ((${#missing[@]})); then
  printf 'Missing required toolchain(s): %s\n' "${missing[*]}" >&2
  printf '%s\n' 'Install trusted distro packages/toolchains and rerun build_all.sh; this script does not execute untrusted installers as root.' >&2
  exit 2
fi
cargo test --manifest-path "$ROOT/daemon/Cargo.toml"
cargo build --manifest-path "$ROOT/daemon/Cargo.toml" --release
cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build" --target openbiounlock-desktop openbiounlock-daemon openbiounlock-pam
flutter pub get --directory "$ROOT/mobile"
flutter analyze --directory "$ROOT/mobile"
flutter test --directory "$ROOT/mobile"
printf '%s\n' 'OpenBioUnlock build and test checks passed.'
