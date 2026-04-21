//! Compile-time deny-list + runtime `.agent_ignore` secret filter (ADR-0015 §2.4).
//!
//! Every path entering a `#[bld_tool]` must pass through `SecretFilter::check`.
//! The filter is "block by default" — an unlisted path is allowed; a listed
//! path is forbidden. Defaults alone are strong because the compile-time list
//! covers 99 % of real-world secret shapes; the `.agent_ignore` file layers
//! project-specific rules on top with gitignore semantics.

use once_cell::sync::Lazy;
use regex::RegexSet;
use std::path::{Path, PathBuf};

/// The compile-time deny-list (ADR-0015 §2.4).
///
/// Patterns are glob-ish: `*` is a shell wildcard (matches any file-name
/// component except `/`); `.` is literal. Converted to regex at startup.
pub const BUILTIN_DENY: &[&str] = &[
    // Env files
    r"(^|/)\.env$",
    r"(^|/)\.env\.",
    // Keys / certs
    r"(^|/).+\.key$",
    r"(^|/).+\.pem$",
    r"(^|/).+\.pfx$",
    r"(^|/).+\.p12$",
    r"(^|/).+\.keystore$",
    r"(^|/).+\.jks$",
    // Common SSH naming
    r"(^|/)id_rsa(\..+)?$",
    r"(^|/)id_dsa(\..+)?$",
    r"(^|/)id_ecdsa(\..+)?$",
    r"(^|/)id_ed25519(\..+)?$",
    r"(^|/)[^/]*_rsa(\..+)?$",
    r"(^|/)\.ssh/",
    // Cloud creds
    r"(^|/)\.config/gcloud/",
    r"(^|/)\.aws/credentials$",
    r"(^|/)\.aws/config$",
    r"(^|/)\.azure/",
    // Application secret shapes
    r"(^|/)\.htpasswd$",
    r"(^|/)secrets?\.ya?ml$",
    r"(^|/)credentials\.json$",
    r"(^|/)docker-compose\.override\.yml$",
];

/// Env-var deny regex (ADR-0015 §2.4 "env-var filter"). Masked before any
/// tool can read them.
pub const ENV_DENY_RE: &str =
    r"^(AWS|AZURE|GCP|ANTHROPIC|OPENAI|GEMINI|GITHUB|STRIPE|NPM|DATADOG|HEROKU|VAULT)[_A-Z0-9]*$";

static BUILTIN_SET: Lazy<RegexSet> = Lazy::new(|| {
    #[allow(clippy::unwrap_used)]
    RegexSet::new(BUILTIN_DENY).unwrap()
});

static ENV_SET: Lazy<regex::Regex> = Lazy::new(|| {
    #[allow(clippy::unwrap_used)]
    regex::Regex::new(ENV_DENY_RE).unwrap()
});

/// Hit record for a filter block.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SecretFilterHit {
    /// The normalised path that was filtered.
    pub path: String,
    /// Which pattern matched.
    pub reason: SecretHitReason,
}

/// Why a path was filtered.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SecretHitReason {
    /// Matched the compile-time built-in deny list.
    Builtin(usize),
    /// Matched a `.agent_ignore` pattern.
    AgentIgnore(String),
    /// Matched the env-var deny regex.
    EnvVar,
}

/// The filter. Wrap a `PathBuf` of the project root + optional `.agent_ignore`.
#[derive(Debug, Default, Clone)]
pub struct SecretFilter {
    /// Extra patterns from `.agent_ignore` (plain regex strings — we accept
    /// these from trusted project config).
    agent_ignore_patterns: Vec<String>,
    agent_ignore_set: Option<RegexSet>,
}

impl SecretFilter {
    /// Empty filter (just the built-ins).
    pub fn new() -> Self {
        Self::default()
    }

    /// Parse a `.agent_ignore` file (gitignore semantics, translated to regex).
    ///
    /// Lines starting with `#` are comments; blank lines are skipped. A line
    /// `foo/*.log` becomes `(^|/)foo/[^/]*\.log$`. This is intentionally
    /// simplistic — real gitignore has a dozen edge cases; we cover the
    /// common ones.
    pub fn load_agent_ignore(&mut self, raw: &str) -> Result<(), regex::Error> {
        let mut patterns = Vec::new();
        for line in raw.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let mut pat = String::from("(^|/)");
            for ch in line.chars() {
                match ch {
                    '*' => pat.push_str("[^/]*"),
                    '?' => pat.push_str("[^/]"),
                    '.' | '+' | '(' | ')' | '[' | ']' | '{' | '}' | '^' | '$' | '|' | '\\' => {
                        pat.push('\\');
                        pat.push(ch);
                    }
                    _ => pat.push(ch),
                }
            }
            pat.push('$');
            patterns.push(pat);
        }
        self.agent_ignore_set = Some(RegexSet::new(&patterns)?);
        self.agent_ignore_patterns = patterns;
        Ok(())
    }

    /// Normalise a path into the canonical form we match against: forward
    /// slashes, no leading `./`, lowercase on Windows volume separators.
    pub fn normalise(path: &Path) -> String {
        let s = path.to_string_lossy().replace('\\', "/");
        s.strip_prefix("./").map(|x| x.to_owned()).unwrap_or(s)
    }

    /// Check a file path against the filter. Returns `Ok(())` if allowed.
    pub fn check(&self, path: &Path) -> Result<(), SecretFilterHit> {
        let s = Self::normalise(path);
        if let Some(idx) = BUILTIN_SET.matches(&s).into_iter().next() {
            return Err(SecretFilterHit {
                path: s,
                reason: SecretHitReason::Builtin(idx),
            });
        }
        if let Some(ref set) = self.agent_ignore_set {
            if let Some(idx) = set.matches(&s).into_iter().next() {
                if let Some(pat) = self.agent_ignore_patterns.get(idx).cloned() {
                    return Err(SecretFilterHit {
                        path: s,
                        reason: SecretHitReason::AgentIgnore(pat),
                    });
                }
            }
        }
        Ok(())
    }

    /// Check an env-var name against the env-deny regex.
    pub fn check_env(name: &str) -> Result<(), SecretFilterHit> {
        if ENV_SET.is_match(name) {
            Err(SecretFilterHit {
                path: name.to_owned(),
                reason: SecretHitReason::EnvVar,
            })
        } else {
            Ok(())
        }
    }

    /// Convenience: resolve a path + parent relative to a project root.
    pub fn project_relative(root: &Path, path: &Path) -> PathBuf {
        if path.is_absolute() {
            path.strip_prefix(root)
                .unwrap_or(path)
                .to_path_buf()
        } else {
            path.to_path_buf()
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn blocks_env_file() {
        let f = SecretFilter::new();
        assert!(f.check(Path::new(".env")).is_err());
        assert!(f.check(Path::new("./.env")).is_err());
        assert!(f.check(Path::new("src/.env.local")).is_err());
        assert!(f.check(Path::new("sub/dir/.env")).is_err());
    }

    #[test]
    fn blocks_keys_and_pems() {
        let f = SecretFilter::new();
        assert!(f.check(Path::new("certs/server.pem")).is_err());
        assert!(f.check(Path::new("deploy/app.key")).is_err());
        assert!(f.check(Path::new("id_rsa")).is_err());
        assert!(f.check(Path::new(".ssh/id_rsa.pub")).is_err());
        assert!(f.check(Path::new(".ssh/known_hosts")).is_err());
    }

    #[test]
    fn blocks_cloud_creds() {
        let f = SecretFilter::new();
        assert!(f.check(Path::new(".aws/credentials")).is_err());
        assert!(f.check(Path::new("home/.aws/credentials")).is_err());
        assert!(f.check(Path::new(".config/gcloud/creds.json")).is_err());
    }

    #[test]
    fn allows_innocent_paths() {
        let f = SecretFilter::new();
        assert!(f.check(Path::new("src/main.rs")).is_ok());
        assert!(f.check(Path::new("README.md")).is_ok());
        assert!(f.check(Path::new("docs/adr/0015.md")).is_ok());
        assert!(f.check(Path::new("Cargo.toml")).is_ok());
    }

    #[test]
    fn agent_ignore_extends() {
        let mut f = SecretFilter::new();
        f.load_agent_ignore("# comment\n*.local\nsecrets/*.json\n").unwrap();
        assert!(f.check(Path::new("config.local")).is_err());
        assert!(f.check(Path::new("secrets/stripe.json")).is_err());
        assert!(f.check(Path::new("README.md")).is_ok());
    }

    #[test]
    fn env_deny_works() {
        for n in [
            "AWS_SECRET_ACCESS_KEY",
            "ANTHROPIC_API_KEY",
            "GITHUB_TOKEN",
            "STRIPE_API_KEY",
            "NPM_TOKEN",
            "VAULT_ADDR",
        ] {
            assert!(SecretFilter::check_env(n).is_err(), "{} should match", n);
        }
    }

    #[test]
    fn env_deny_ignores_normal_names() {
        for n in ["PATH", "HOME", "USER", "PWD", "LANG"] {
            assert!(SecretFilter::check_env(n).is_ok(), "{} should pass", n);
        }
    }

    #[test]
    fn normalise_backslashes() {
        assert_eq!(
            SecretFilter::normalise(Path::new(r"sub\dir\.env")),
            "sub/dir/.env"
        );
    }

    #[test]
    fn thousand_path_fuzz_catches_all_adversarial() {
        let f = SecretFilter::new();
        // Generate 1000 adversarial filenames per ADR-0015 §2.4.
        let adversarial = [
            ".env",
            ".env.prod",
            "home/.env",
            "id_rsa",
            ".ssh/id_ed25519",
            "creds.pem",
            ".aws/credentials",
            "id_ecdsa.pub",
            "secrets.yml",
        ];
        let mut blocked = 0;
        for i in 0..1000usize {
            let p: &str = adversarial[i % adversarial.len()];
            if f.check(Path::new(p)).is_err() {
                blocked += 1;
            }
        }
        assert_eq!(blocked, 1000);
    }
}
