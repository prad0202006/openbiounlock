use keyring::Entry;
use thiserror::Error;

const SERVICE: &str = "openbiounlock";
const DEVICE_KEYS: &str = "paired-device-public-keys";

#[derive(Debug, Error)]
pub enum StoreError {
    #[error("credential vault error: {0}")]
    Vault(#[from] keyring::Error),
    #[error("stored key data is invalid: {0}")]
    Encoding(#[from] hex::FromHexError),
}

pub struct KeyStore {
    entry: Entry,
}

impl KeyStore {
    pub fn open() -> Result<Self, StoreError> {
        Ok(Self { entry: Entry::new(SERVICE, DEVICE_KEYS)? })
    }

    pub fn load_public_keys(&self) -> Result<Vec<[u8; 32]>, StoreError> {
        let Ok(value) = self.entry.get_password() else { return Ok(Vec::new()) };
        value.lines().filter(|line| !line.is_empty()).map(|line| {
            let bytes = hex::decode(line)?;
            bytes.as_slice().try_into().map_err(|_| hex::FromHexError::InvalidStringLength)
        }).collect()
    }

    pub fn add_public_key(&self, key: &[u8; 32]) -> Result<(), StoreError> {
        let mut keys = self.load_public_keys()?;
        if !keys.iter().any(|known| known == key) { keys.push(*key); }
        let value = keys.iter().map(hex::encode).collect::<Vec<_>>().join("\n");
        self.entry.set_password(&value)?;
        Ok(())
    }
}
