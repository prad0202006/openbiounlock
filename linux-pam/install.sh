#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cargo build --manifest-path "$ROOT/daemon/Cargo.toml" --release
install -Dm755 "$ROOT/daemon/target/release/openbiounlock-daemon" /usr/local/bin/openbiounlock-daemon
make -C "$ROOT/linux-pam"
install -Dm755 "$ROOT/linux-pam/pam_openbiounlock.so" /lib/security/pam_openbiounlock.so
install -d -m 0750 /run/openbiounlock
if ! grep -q '^auth.*pam_openbiounlock.so' /etc/pam.d/sudo; then
  printf '%s\n' 'auth sufficient pam_openbiounlock.so' >> /etc/pam.d/sudo
fi
printf '%s\n' 'Installed OpenBioUnlock daemon and PAM module.'
