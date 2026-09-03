use crate::crypto::{random_nonce, verify_signature};
use crate::store::KeyStore;
use serde::{Deserialize, Serialize};
use std::time::{SystemTime, UNIX_EPOCH};
use std::{collections::HashMap, sync::Arc};
use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::sync::{Mutex, Notify};
use tokio::time::{timeout, Duration};
use tokio::net::TcpListener;
#[cfg(unix)]
use tokio::net::UnixListener;
use tracing::{info, warn};
#[cfg(windows)]
use tokio::net::windows::named_pipe::ServerOptions;

#[derive(Debug, Deserialize)]
#[serde(tag = "type")]
pub enum Request {
    #[serde(rename = "pair")]
    Pair { device_id: String, public_key: String, pairing_code: String },
    #[serde(rename = "authorize")]
    Authorize { user: String },
    #[serde(rename = "challenge")]
    Challenge,
    #[serde(rename = "verify")]
    Verify { challenge_id: String, device_id: String, nonce: String, timestamp: u64, signature: String },
}

#[derive(Debug, Serialize)]
#[serde(tag = "type")]
pub enum Response {
    #[serde(rename = "paired")]
    Paired { accepted: bool },
    #[serde(rename = "challenge")]
    Challenge { challenge_id: String, nonce: String, timestamp: u64 },
    #[serde(rename = "authorized")]
    Authorized { authorized: bool },
    #[serde(rename = "error")]
    Error { message: String },
}

fn now() -> u64 { SystemTime::now().duration_since(UNIX_EPOCH).expect("clock before epoch").as_secs() }

struct PendingAuth {
    nonce: [u8; 32],
    timestamp: u64,
    authorized: bool,
    notify: Arc<Notify>,
}

pub type SharedState = Arc<Mutex<HashMap<String, PendingAuth>>>;

pub fn new_state() -> SharedState { Arc::new(Mutex::new(HashMap::new())) }

pub async fn run_tcp(address: &str, state: SharedState) -> std::io::Result<()> {
    let listener = TcpListener::bind(address).await?;
    info!(%address, "daemon TCP listener ready");
    loop {
        let (stream, _) = listener.accept().await?;
        tokio::spawn(handle_stream(stream, state.clone()));
    }
}

#[cfg(windows)]
pub async fn run_named_pipe(address: &str, state: SharedState) -> std::io::Result<()> {
    loop {
        let server = ServerOptions::new().create(address)?;
        server.connect().await?;
        tokio::spawn(handle_stream(server, state.clone()));
    }
}

#[cfg(unix)]
pub async fn run_unix(address: &str, state: SharedState) -> std::io::Result<()> {
    let _ = std::fs::remove_file(address);
    let listener = UnixListener::bind(address)?;
    info!(%address, "daemon Unix socket listener ready");
    loop {
        let (stream, _) = listener.accept().await?;
        tokio::spawn(handle_stream(stream, state.clone()));
    }
}

async fn handle_stream<S>(stream: S, state: SharedState)
where
    S: tokio::io::AsyncRead + tokio::io::AsyncWrite + Unpin,
{
    let store = KeyStore::open();
    let (reader, mut writer) = tokio::io::split(stream);
    let mut lines = BufReader::new(reader).lines();
    while let Ok(Some(line)) = lines.next_line().await {
        let response = match serde_json::from_str::<Request>(&line) {
                    Ok(Request::Pair { device_id: _, public_key, pairing_code }) => {
                        if pairing_code.is_empty() {
                            Response::Error { message: "pairing code is required".to_string() }
                        } else {
                        match (store.as_ref(), hex::decode(public_key)) {
                            (Ok(store), Ok(bytes)) => match bytes.as_slice().try_into() {
                                Ok(key) => Response::Paired { accepted: store.add_public_key(&key).is_ok() },
                                Err(_) => Response::Error { message: "public key must be 32 bytes".to_string() },
                            },
                            _ => Response::Error { message: "pairing key is invalid or vault is unavailable".to_string() },
                        }
                        }
                    },
                    Ok(Request::Authorize { user: _ }) => match random_nonce() {
                        Ok(nonce) => {
                            let challenge_id = hex::encode(random_nonce().unwrap_or([0; 32]));
                            let notify = Arc::new(Notify::new());
                            let pending = PendingAuth { nonce, timestamp: now(), authorized: false, notify: notify.clone() };
                            state.lock().await.insert(challenge_id.clone(), pending);
                            let authorized = timeout(Duration::from_secs(30), async {
                                loop {
                                    notify.notified().await;
                                    if state.lock().await.get(&challenge_id).map(|entry| entry.authorized).unwrap_or(false) { break true; }
                                }
                            }).await.unwrap_or(false);
                            state.lock().await.remove(&challenge_id);
                            Response::Authorized { authorized }
                        }
                        Err(error) => Response::Error { message: error.to_string() },
                    },
                    Ok(Request::Challenge) => {
                        let pending = state.lock().await;
                        if let Some((challenge_id, entry)) = pending.iter().next() {
                            Response::Challenge { challenge_id: challenge_id.clone(), nonce: hex::encode(entry.nonce), timestamp: entry.timestamp }
                        } else {
                            Response::Error { message: "no workstation authentication request is pending".to_string() }
                        }
                    },
                    Ok(Request::Verify { challenge_id, device_id: _, nonce, timestamp, signature }) => {
                        let keys = store.as_ref().ok().and_then(|value| value.load_public_keys().ok()).unwrap_or_default();
                        let nonce = hex::decode(nonce).unwrap_or_default();
                        let signature = hex::decode(signature).unwrap_or_default();
                        let authorized = keys.iter().any(|key| verify_signature(key, &nonce, timestamp, &signature, now()).is_ok()) && {
                            let mut pending = state.lock().await;
                            if let Some(entry) = pending.get_mut(&challenge_id) {
                                let matches = entry.nonce.as_slice() == nonce.as_slice() && entry.timestamp == timestamp && !entry.authorized;
                                if matches { entry.authorized = true; entry.notify.notify_waiters(); }
                                matches
                            } else { false }
                        };
                        Response::Authorized { authorized }
                    }
                    Err(error) => Response::Error { message: format!("invalid request: {error}") },
                };
        let mut encoded = serde_json::to_vec(&response).expect("response serialization");
        encoded.push(b'\n');
        if writer.write_all(&encoded).await.is_err() { break; }
    }
    warn!("daemon client disconnected");
}
