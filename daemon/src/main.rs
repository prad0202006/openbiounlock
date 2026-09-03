mod crypto;
mod server;
mod store;

use std::time::{SystemTime, UNIX_EPOCH};
use tracing::info;

fn unix_time() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).expect("system clock before Unix epoch").as_secs()
}

#[tokio::main]
async fn main() -> std::io::Result<()> {
    tracing_subscriber::fmt().with_env_filter("info").init();
    info!(version = env!("CARGO_PKG_VERSION"), now = unix_time(), "OpenBioUnlock daemon starting");
    match store::KeyStore::open() {
        Ok(store) => match store.load_public_keys() {
            Ok(keys) => info!(paired_devices = keys.len(), "loaded paired device keys"),
            Err(error) => tracing::error!(%error, "could not load paired device keys"),
        },
        Err(error) => tracing::error!(%error, "could not open OS credential vault"),
    }
    let state = server::new_state();
    #[cfg(unix)]
    {
        tokio::try_join!(server::run_tcp("127.0.0.1:45821", state.clone()), server::run_unix("/var/run/openbiounlock.sock", state.clone()))?;
        Ok(())
    }
    #[cfg(not(unix))]
    {
        tokio::try_join!(server::run_tcp("127.0.0.1:45821", state.clone()), server::run_named_pipe(r"\\.\pipe\OpenBioUnlockPipe", state))?;
        Ok(())
    }
}
