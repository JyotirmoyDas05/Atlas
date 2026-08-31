//! Manual smoke test: `cargo run --release --example smoke -- <query>`
//! Starts the engine against a temp data dir, waits for the build, prints
//! status and top search results with timings.

use std::ffi::{CStr, CString};
use std::time::{Duration, Instant};

use atlas_indexer::*;

fn take(ptr: *mut std::ffi::c_char) -> String {
    let s = unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned();
    atlas_free_string(ptr);
    s
}

fn main() {
    let query = std::env::args().nth(1).unwrap_or_else(|| "cargo.toml".into());
    let data_dir = std::env::temp_dir().join("atlas-indexer-smoke");
    let config = format!(r#"{{"data_dir":{:?}}}"#, data_dir.to_string_lossy());

    let rc = atlas_indexer_start(CString::new(config).unwrap().into_raw());
    println!("start rc={rc}");

    let started = Instant::now();
    loop {
        let status = take(atlas_indexer_status_json());
        println!("[{:>6.1}s] {status}", started.elapsed().as_secs_f32());
        if status.contains(r#""state":"ready""#) && !status.contains(r#""source":"snapshot""#) {
            break;
        }
        if started.elapsed() > Duration::from_secs(300) {
            println!("timeout waiting for index");
            return;
        }
        std::thread::sleep(Duration::from_millis(500));
    }

    for _ in 0..3 {
        let t = Instant::now();
        let results = take(atlas_indexer_search_json(
            CString::new(query.clone()).unwrap().into_raw(),
            10,
        ));
        println!("search '{query}' took {:?}", t.elapsed());
        for line in results.split("},{").take(10) {
            println!("  {line}");
        }
    }

    // Verify the change watcher: touch a file in the indexed tree and wait for
    // the debounced rebuild to bump the generation counter.
    if std::env::args().nth(2).as_deref() == Some("--watch-test") {
        let home = std::env::var("USERPROFILE").expect("USERPROFILE");
        let marker = std::path::Path::new(&home).join("atlas-watch-test.tmp");
        std::fs::write(&marker, "x").expect("write marker");
        println!("touched {marker:?}; waiting for rebuild (debounce 15s + walk)...");
        let waited = Instant::now();
        loop {
            let status = take(atlas_indexer_status_json());
            if status.contains(r#""generation":2"#) {
                println!("[{:>6.1}s] rebuilt: {status}", waited.elapsed().as_secs_f32());
                break;
            }
            if waited.elapsed() > Duration::from_secs(300) {
                println!("timeout: {status}");
                break;
            }
            std::thread::sleep(Duration::from_secs(2));
        }
        let _ = std::fs::remove_file(&marker);
        let results = take(atlas_indexer_search_json(
            CString::new("atlas-watch-test").unwrap().into_raw(),
            5,
        ));
        println!("search for marker file: {results}");
    }
}
