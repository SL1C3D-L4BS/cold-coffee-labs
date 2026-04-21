//! File-watcher hook.
//!
//! The real watcher drives incremental indexing from a `notify` crate
//! stream; Phase 9C ships the trait + a synchronous `ManualWatcher` so
//! tests can trigger events deterministically. Wave 9D replaces the
//! backend with a `notify::Watcher` without touching the retrieval API.

use std::path::PathBuf;

/// A single filesystem event the watcher emits.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum WatchEvent {
    /// A file was created or modified; re-ingest it.
    Upsert(PathBuf),
    /// A file was deleted; drop it from the corpus.
    Remove(PathBuf),
}

/// Watcher trait. Implementations push events into a queue the indexer
/// drains on its next tick.
pub trait Watcher {
    /// Drain pending events.
    fn drain(&mut self) -> Vec<WatchEvent>;
}

/// Synchronous test watcher — callers push events in directly.
#[derive(Debug, Default)]
pub struct ManualWatcher {
    events: Vec<WatchEvent>,
}

impl ManualWatcher {
    /// Empty watcher.
    pub fn new() -> Self {
        Self::default()
    }

    /// Record an event.
    pub fn push(&mut self, ev: WatchEvent) {
        self.events.push(ev);
    }

    /// True if there are no pending events.
    pub fn is_empty(&self) -> bool {
        self.events.is_empty()
    }
}

impl Watcher for ManualWatcher {
    fn drain(&mut self) -> Vec<WatchEvent> {
        std::mem::take(&mut self.events)
    }
}

// -----------------------------------------------------------------------------
// notify-backed watcher (feature-gated).
// -----------------------------------------------------------------------------

/// Real `notify`-backed watcher. Only compiled when the `fs-notify`
/// feature is on.
#[cfg(feature = "fs-notify")]
pub mod notify_backend {
    use super::{WatchEvent, Watcher};
    use std::path::{Path, PathBuf};
    use std::sync::{Arc, Mutex};
    use std::time::Duration;

    use notify::RecursiveMode;
    use notify_debouncer_mini::{new_debouncer, DebouncedEventKind, Debouncer};

    /// Errors produced while creating the watcher.
    #[derive(Debug, thiserror::Error)]
    pub enum FsWatchError {
        /// The underlying `notify` watcher failed to initialise.
        #[error("notify init: {0}")]
        Init(String),
    }

    /// Shared event queue wrapped in a tiny mutex — writes come from the
    /// notify worker thread, reads from the agent loop's turn tick.
    type Queue = Arc<Mutex<Vec<WatchEvent>>>;

    /// A watcher that turns `notify`'s debounced events into the
    /// `WatchEvent` stream the RAG layer consumes.
    pub struct FsWatcher {
        queue: Queue,
        // Hold the debouncer alive so the background thread keeps running.
        _debouncer: Debouncer<notify::RecommendedWatcher>,
    }

    impl FsWatcher {
        /// Watch the given root directory recursively with the supplied
        /// debounce window.
        pub fn new(root: impl AsRef<Path>, debounce: Duration) -> Result<Self, FsWatchError> {
            let queue: Queue = Arc::new(Mutex::new(Vec::new()));
            let q_clone = Arc::clone(&queue);

            let mut debouncer = new_debouncer(debounce, move |res| {
                let events = match res {
                    Ok(evs) => evs,
                    Err(e) => {
                        tracing::warn!(?e, "notify debouncer error");
                        return;
                    }
                };
                let Ok(mut q) = q_clone.lock() else { return };
                for ev in events {
                    let ev: notify_debouncer_mini::DebouncedEvent = ev;
                    match ev.kind {
                        DebouncedEventKind::Any => {
                            let p: PathBuf = ev.path;
                            if p.exists() {
                                q.push(WatchEvent::Upsert(p));
                            } else {
                                q.push(WatchEvent::Remove(p));
                            }
                        }
                        _ => {}
                    }
                }
            })
            .map_err(|e| FsWatchError::Init(e.to_string()))?;
            debouncer
                .watcher()
                .watch(root.as_ref(), RecursiveMode::Recursive)
                .map_err(|e| FsWatchError::Init(e.to_string()))?;
            Ok(Self {
                queue,
                _debouncer: debouncer,
            })
        }

        /// Queue size. Useful for tests + telemetry.
        pub fn pending(&self) -> usize {
            self.queue.lock().map(|q| q.len()).unwrap_or(0)
        }
    }

    impl Watcher for FsWatcher {
        fn drain(&mut self) -> Vec<WatchEvent> {
            let Ok(mut q) = self.queue.lock() else { return Vec::new() };
            std::mem::take(&mut *q)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn manual_watcher_drains_fifo() {
        let mut w = ManualWatcher::new();
        w.push(WatchEvent::Upsert(PathBuf::from("a.md")));
        w.push(WatchEvent::Remove(PathBuf::from("b.md")));
        let out = w.drain();
        assert_eq!(out.len(), 2);
        assert!(w.is_empty());
    }
}
