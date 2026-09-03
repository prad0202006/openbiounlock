use ed25519_dalek::{Signature, Verifier, VerifyingKey};
use thiserror::Error;

pub const NONCE_LEN: usize = 32;
pub const FRESHNESS_WINDOW_SECONDS: u64 = 30;

#[derive(Debug, Error)]
pub enum CryptoError {
    #[error("secure random generation failed: {0}")]
    Random(#[from] getrandom::Error),
    #[error("invalid public key")]
    InvalidPublicKey,
    #[error("invalid signature")]
    InvalidSignature,
    #[error("timestamp is outside the freshness window")]
    StaleTimestamp,
}

pub fn random_nonce() -> Result<[u8; NONCE_LEN], CryptoError> {
    let mut nonce = [0_u8; NONCE_LEN];
    getrandom::getrandom(&mut nonce)?;
    Ok(nonce)
}

pub fn is_fresh(timestamp: u64, now: u64) -> bool {
    timestamp.abs_diff(now) <= FRESHNESS_WINDOW_SECONDS
}

pub fn verify_signature(
    public_key: &[u8],
    nonce: &[u8],
    timestamp: u64,
    signature: &[u8],
    now: u64,
) -> Result<(), CryptoError> {
    if nonce.len() != NONCE_LEN || !is_fresh(timestamp, now) {
        return Err(CryptoError::StaleTimestamp);
    }
    let key_bytes: [u8; 32] = public_key.try_into().map_err(|_| CryptoError::InvalidPublicKey)?;
    let verifying_key = VerifyingKey::from_bytes(&key_bytes).map_err(|_| CryptoError::InvalidPublicKey)?;
    let signature = Signature::from_slice(signature).map_err(|_| CryptoError::InvalidSignature)?;
    let mut payload = [0_u8; 40];
    payload[..NONCE_LEN].copy_from_slice(nonce);
    payload[NONCE_LEN..].copy_from_slice(&timestamp.to_be_bytes());
    verifying_key.verify(&payload, &signature).map_err(|_| CryptoError::InvalidSignature)
}

#[cfg(test)]
mod tests {
    use super::*;
    use ed25519_dalek::{Signer, SigningKey};

    #[test]
    fn verifies_current_challenge_and_rejects_stale_one() {
        let signing = SigningKey::from_bytes(&[7_u8; 32]);
        let nonce = [3_u8; NONCE_LEN];
        let timestamp = 1_000;
        let mut payload = nonce.to_vec();
        payload.extend_from_slice(&timestamp.to_be_bytes());
        let signature = signing.sign(&payload);
        assert!(verify_signature(signing.verifying_key().as_bytes(), &nonce, timestamp, signature.to_bytes().as_slice(), timestamp).is_ok());
        assert!(verify_signature(signing.verifying_key().as_bytes(), &nonce, timestamp, signature.to_bytes().as_slice(), timestamp + 31).is_err());
    }

    #[test]
    fn rejects_fake_64_byte_signature() {
        let nonce = [1_u8; NONCE_LEN];
        let fake_signature = [0xaa_u8; 64];
        assert!(verify_signature(&[2_u8; 32], &nonce, 1_000, &fake_signature, 1_000).is_err());
    }
}
