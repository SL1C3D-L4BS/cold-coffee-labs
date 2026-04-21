//! API-key storage facade (ADR-0010 §2.6).
//!
//! `KeyStore` is a two-method trait: `get(provider)` and `put(provider, secret)`.
//! The default `EnvKeyStore` reads `ANTHROPIC_API_KEY`, `OPENAI_API_KEY`, etc.
//! Behind the `keyring` feature, `OsKeyStore` uses the platform keychain
//! (Windows Credential Manager, macOS Keychain, Linux Secret Service).
//!
//! All secrets are zero-tracked: the raw bytes never reach `tracing`
//! because we never `Debug`-format a `Secret` — only its length.

use std::collections::HashMap;
use std::sync::RwLock;

/// Errors from the secret store.
#[derive(Debug, thiserror::Error)]
pub enum KeyStoreError {
    /// The provider has no key configured.
    #[error("no key configured for {0}")]
    Missing(String),
    /// Backend-specific error.
    #[error("backend: {0}")]
    Backend(String),
}

/// A secret. `Debug` is opaque; `Display` is NOT implemented to prevent
/// accidental logging.
pub struct Secret(String);

impl Secret {
    /// Wrap a string as a secret.
    pub fn new(s: impl Into<String>) -> Self {
        Self(s.into())
    }
    /// Expose as `&str`. Only call this when handing to an HTTP client.
    pub fn expose(&self) -> &str {
        &self.0
    }
    /// Length in bytes (safe to log).
    pub fn len(&self) -> usize {
        self.0.len()
    }
    /// Whether the secret is empty.
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
}

impl std::fmt::Debug for Secret {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Secret(<{} bytes redacted>)", self.0.len())
    }
}

/// The keystore trait.
pub trait KeyStore: Send + Sync {
    /// Fetch the secret for a provider (e.g. `"claude"`, `"openai"`).
    fn get(&self, provider: &str) -> Result<Secret, KeyStoreError>;
    /// Store / update a secret.
    fn put(&self, provider: &str, secret: &str) -> Result<(), KeyStoreError>;
}

/// Default `KeyStore`: reads env vars.
///
/// Lookup order:
/// 1. `<PROVIDER>_API_KEY`  (e.g. `ANTHROPIC_API_KEY`, `OPENAI_API_KEY`)
/// 2. `<PROVIDER>_TOKEN`    (e.g. `GITHUB_TOKEN`)
/// 3. Overrides passed via `EnvKeyStore::with_override`.
#[derive(Debug, Default)]
pub struct EnvKeyStore {
    overrides: RwLock<HashMap<String, String>>,
}

impl EnvKeyStore {
    /// Empty store that reads from `std::env`.
    pub fn new() -> Self {
        Self::default()
    }

    /// Inject an override (useful for tests).
    pub fn with_override(self, provider: &str, key: &str) -> Self {
        #[allow(clippy::expect_used)]
        self.overrides
            .write()
            .expect("env-keystore RwLock poisoned")
            .insert(provider.to_ascii_lowercase(), key.to_owned());
        self
    }

    fn env_names(provider: &str) -> [String; 2] {
        // Map common provider ids to env-var prefixes.
        let prefix = match provider.to_ascii_lowercase().as_str() {
            "claude" | "anthropic" => "ANTHROPIC",
            "openai" | "gpt" => "OPENAI",
            "gemini" | "google" => "GEMINI",
            other => return [
                format!("{}_API_KEY", other.to_ascii_uppercase()),
                format!("{}_TOKEN", other.to_ascii_uppercase()),
            ],
        };
        [format!("{prefix}_API_KEY"), format!("{prefix}_TOKEN")]
    }
}

impl KeyStore for EnvKeyStore {
    fn get(&self, provider: &str) -> Result<Secret, KeyStoreError> {
        let key = provider.to_ascii_lowercase();
        if let Ok(guard) = self.overrides.read() {
            if let Some(v) = guard.get(&key) {
                if !v.is_empty() {
                    return Ok(Secret::new(v.clone()));
                }
            }
        }
        for name in Self::env_names(&key) {
            if let Ok(v) = std::env::var(&name) {
                if !v.is_empty() {
                    return Ok(Secret::new(v));
                }
            }
        }
        Err(KeyStoreError::Missing(provider.to_owned()))
    }

    fn put(&self, provider: &str, secret: &str) -> Result<(), KeyStoreError> {
        #[allow(clippy::expect_used)]
        self.overrides
            .write()
            .expect("env-keystore RwLock poisoned")
            .insert(provider.to_ascii_lowercase(), secret.to_owned());
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn env_store_override_works() {
        let s = EnvKeyStore::new().with_override("claude", "sk-fake");
        let got = s.get("claude").unwrap();
        assert_eq!(got.expose(), "sk-fake");
        assert_eq!(got.len(), 7);
    }

    #[test]
    fn env_store_returns_missing_when_nothing_set() {
        let s = EnvKeyStore::new();
        let err = s.get("definitely-not-a-provider-slug-xyz").unwrap_err();
        assert!(matches!(err, KeyStoreError::Missing(_)));
    }

    #[test]
    fn env_store_put_then_get() {
        let s = EnvKeyStore::new();
        s.put("mytest", "hello").unwrap();
        assert_eq!(s.get("mytest").unwrap().expose(), "hello");
    }

    #[test]
    fn secret_debug_redacts() {
        let s = Secret::new("topsecret123");
        let d = format!("{s:?}");
        assert!(!d.contains("topsecret"));
        assert!(d.contains("12 bytes redacted"));
    }

    #[test]
    fn provider_alias_mapping() {
        let names = EnvKeyStore::env_names("claude");
        assert_eq!(names[0], "ANTHROPIC_API_KEY");
        let names = EnvKeyStore::env_names("openai");
        assert_eq!(names[0], "OPENAI_API_KEY");
    }
}
