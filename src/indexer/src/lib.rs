//! Atlas file indexer: C FFI surface consumed by the Qt host.
//!
//! Lifecycle: `atlas_indexer_start(config_json)` loads any snapshot (instant),
//! then rebuilds in the background — USN/MFT enumeration when the process can
//! open raw volume handles, parallel directory walk otherwise — and swaps the
//! fresh index in when done. `atlas_indexer_search_json` is safe to call at
//! any point; it sees whichever index is current.

mod engine;
mod store;
#[cfg(windows)]
mod usn;
mod walker;
#[cfg(windows)]
mod watch;

use std::ffi::{c_char, CStr, CString};
use std::path::PathBuf;
use std::sync::{Arc, Mutex, OnceLock, RwLock};
use std::time::Instant;

use serde::{Deserialize, Serialize};

use engine::FileIndex;
use store::Frecency;

#[derive(Deserialize)]
struct StartConfig {
    data_dir: String,
    #[serde(default)]
    roots: Vec<String>,
    #[serde(default)]
    skip_dirs: Vec<String>,
}

#[derive(Serialize, Clone, Default)]
struct Status {
    state: String,  // "starting" | "indexing" | "ready" | "error"
    source: String, // "snapshot" | "usn" | "walker" | ""
    watch: String,  // "usn-journal" | "rdcw" | "none" | ""
    file_count: usize,
    build_ms: u64,
    generation: u64, // completed (re)builds this session
    error: String,
}

struct EngineState {
    index: RwLock<Arc<FileIndex>>,
    frecency: Mutex<Frecency>,
    status: Mutex<Status>,
    data_dir: PathBuf,
}

static ENGINE: OnceLock<Arc<EngineState>> = OnceLock::new();

fn to_c_string(s: String) -> *mut c_char {
    CString::new(s.into_bytes())
        .unwrap_or_else(|_| CString::new("[]").unwrap())
        .into_raw()
}

fn parse_c_str<'a>(ptr: *const c_char) -> Option<&'a str> {
    if ptr.is_null() {
        return None;
    }
    unsafe { CStr::from_ptr(ptr) }.to_str().ok()
}

// Replaces the status but preserves the fields that outlive individual
// rebuilds (watch mode, rebuild generation).
fn set_status(state: &EngineState, status: Status) {
    let mut current = state.status.lock().unwrap();
    let watch = std::mem::take(&mut current.watch);
    let generation = current.generation;
    *current = status;
    current.watch = watch;
    current.generation = generation;
}

fn default_roots() -> Vec<PathBuf> {
    std::env::var("USERPROFILE")
        .map(|home| vec![PathBuf::from(home)])
        .unwrap_or_default()
}

fn rebuild(state: &EngineState, roots: Vec<PathBuf>, skip_dirs: Vec<String>) {
    let started = Instant::now();
    set_status(
        state,
        Status {
            state: "indexing".into(),
            file_count: state.index.read().unwrap().len(),
            ..Default::default()
        },
    );

    #[cfg(windows)]
    let usn_result = usn::build_all();
    #[cfg(not(windows))]
    let usn_result: Result<FileIndex, String> = Err("not windows".into());

    let (index, source) = match usn_result {
        Ok(index) if index.len() > 0 => (index, "usn"),
        _ => {
            let skip: Vec<String> = if skip_dirs.is_empty() {
                walker::DEFAULT_SKIP_DIRS.iter().map(|s| s.to_string()).collect()
            } else {
                skip_dirs
            };
            (walker::build(&roots, &skip), "walker")
        }
    };

    let build_ms = started.elapsed().as_millis() as u64;
    let count = index.len();
    let index = Arc::new(index);
    *state.index.write().unwrap() = index.clone();
    set_status(
        state,
        Status {
            state: "ready".into(),
            source: source.into(),
            file_count: count,
            build_ms,
            ..Default::default()
        },
    );
    state.status.lock().unwrap().generation += 1;
    let _ = store::save_snapshot(&state.data_dir, &index);
}

/// Start the engine. Returns 0 on success, negative on bad config.
#[no_mangle]
pub extern "C" fn atlas_indexer_start(config_json: *const c_char) -> i32 {
    let Some(json) = parse_c_str(config_json) else {
        return -1;
    };
    let Ok(config) = serde_json::from_str::<StartConfig>(json) else {
        return -2;
    };
    let data_dir = PathBuf::from(&config.data_dir);

    let state = Arc::new(EngineState {
        index: RwLock::new(Arc::new(FileIndex::empty())),
        frecency: Mutex::new(Frecency::load(&data_dir)),
        status: Mutex::new(Status {
            state: "starting".into(),
            ..Default::default()
        }),
        data_dir,
    });

    if ENGINE.set(state.clone()).is_err() {
        return 0; // already started
    }

    let roots: Vec<PathBuf> = if config.roots.is_empty() {
        default_roots()
    } else {
        config.roots.iter().map(PathBuf::from).collect()
    };
    let skip_dirs = config.skip_dirs;

    std::thread::spawn(move || {
        // Snapshot first: search works within milliseconds of process start.
        if let Ok(snapshot) = store::load_snapshot(&state.data_dir) {
            let count = snapshot.len();
            *state.index.write().unwrap() = Arc::new(snapshot);
            set_status(
                &state,
                Status {
                    state: "ready".into(),
                    source: "snapshot".into(),
                    file_count: count,
                    ..Default::default()
                },
            );
        }
        rebuild(&state, roots.clone(), skip_dirs.clone());

        // Watch for filesystem changes and re-index after a quiet period.
        #[cfg(windows)]
        {
            use std::sync::mpsc::{channel, RecvTimeoutError};
            use std::time::{Duration, Instant};

            let (tx, rx) = channel();
            let mode = watch::spawn(tx, &roots);
            state.status.lock().unwrap().watch = mode.into();
            if mode == "none" {
                return;
            }
            loop {
                if rx.recv().is_err() {
                    return; // all watchers gone
                }
                // Debounce: wait for 15s of quiet, but never defer a rebuild
                // by more than 2 minutes under sustained churn.
                let drain_started = Instant::now();
                loop {
                    match rx.recv_timeout(Duration::from_secs(15)) {
                        Ok(()) if drain_started.elapsed() < Duration::from_secs(120) => continue,
                        Ok(()) => break,
                        Err(RecvTimeoutError::Timeout) => break,
                        Err(RecvTimeoutError::Disconnected) => return,
                    }
                }
                rebuild(&state, roots.clone(), skip_dirs.clone());
            }
        }
    });

    0
}

#[derive(Serialize)]
struct SearchResultItem {
    title: String,
    path: String,
    is_dir: bool,
    score: u32,
}

#[no_mangle]
pub extern "C" fn atlas_indexer_search_json(query_ptr: *const c_char, limit: u32) -> *mut c_char {
    let (Some(query), Some(state)) = (parse_c_str(query_ptr), ENGINE.get()) else {
        return to_c_string("[]".into());
    };
    let limit = limit.clamp(1, 500) as usize;

    let index = state.index.read().unwrap().clone();
    let hits = engine::search(&index, query, limit);

    let frecency = state.frecency.lock().unwrap();
    let mut items: Vec<SearchResultItem> = hits
        .into_iter()
        .filter_map(|hit| {
            let path = index.path(hit.idx)?; // None: junk/broken ancestry
            let score = hit.score + frecency.bonus(&path);
            Some(SearchResultItem {
                title: index.name(hit.idx).to_string(),
                path,
                is_dir: index.is_dir(hit.idx),
                score,
            })
        })
        .collect();
    drop(frecency);

    // Tie-break equal fuzzy scores by shorter path: shallow, user-visible
    // files beat identically-named ones buried in caches.
    items.sort_by(|a, b| {
        b.score
            .cmp(&a.score)
            .then_with(|| a.path.len().cmp(&b.path.len()))
    });
    items.truncate(limit);

    to_c_string(serde_json::to_string(&items).unwrap_or_else(|_| "[]".into()))
}

#[no_mangle]
pub extern "C" fn atlas_indexer_status_json() -> *mut c_char {
    let json = match ENGINE.get() {
        Some(state) => {
            let status = state.status.lock().unwrap().clone();
            serde_json::to_string(&status).unwrap_or_else(|_| "{}".into())
        }
        None => r#"{"state":"stopped"}"#.into(),
    };
    to_c_string(json)
}

#[no_mangle]
pub extern "C" fn atlas_indexer_record_open(path_ptr: *const c_char) {
    let (Some(path), Some(state)) = (parse_c_str(path_ptr), ENGINE.get()) else {
        return;
    };
    let mut frecency = state.frecency.lock().unwrap();
    frecency.record(path);
    frecency.save(&state.data_dir);
}

#[no_mangle]
pub extern "C" fn atlas_free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}
