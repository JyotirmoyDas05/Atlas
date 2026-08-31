//! Core in-memory file index and parallel fuzzy search.

use nucleo_matcher::pattern::{CaseMatching, Normalization, Pattern};
use nucleo_matcher::{Config, Matcher, Utf32Str};
use rayon::prelude::*;

pub const NO_PARENT: u32 = u32::MAX;

/// One file/directory. Names live in a shared arena so 500k+ entries stay compact
/// (~12 bytes/entry + name bytes). Paths are resolved on demand by walking parents.
#[derive(Clone, Copy)]
pub struct Entry {
    pub name_start: u32,
    pub name_len: u16,
    pub is_dir: bool,
    pub parent: u32,
}

pub struct FileIndex {
    names: String,
    entries: Vec<Entry>,
}

pub struct IndexBuilder {
    names: String,
    entries: Vec<Entry>,
}

impl IndexBuilder {
    pub fn new() -> Self {
        Self {
            names: String::with_capacity(16 << 20),
            entries: Vec::with_capacity(1 << 20),
        }
    }

    pub fn add(&mut self, name: &str, parent: u32, is_dir: bool) -> u32 {
        let idx = self.entries.len() as u32;
        let start = self.names.len() as u32;
        self.names.push_str(name);
        self.entries.push(Entry {
            name_start: start,
            name_len: name.len().min(u16::MAX as usize) as u16,
            is_dir,
            parent,
        });
        idx
    }

    pub fn set_parent(&mut self, idx: u32, parent: u32) {
        self.entries[idx as usize].parent = parent;
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn finish(self) -> FileIndex {
        FileIndex {
            names: self.names,
            entries: self.entries,
        }
    }
}

impl FileIndex {
    pub fn empty() -> Self {
        Self {
            names: String::new(),
            entries: Vec::new(),
        }
    }

    pub fn from_parts(names: String, entries: Vec<Entry>) -> Self {
        Self { names, entries }
    }

    pub fn parts(&self) -> (&str, &[Entry]) {
        (&self.names, &self.entries)
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn name(&self, idx: u32) -> &str {
        let e = &self.entries[idx as usize];
        &self.names[e.name_start as usize..e.name_start as usize + e.name_len as usize]
    }

    pub fn is_dir(&self, idx: u32) -> bool {
        self.entries[idx as usize].is_dir
    }

    /// Resolve full path. Returns None for entries whose ancestry crosses
    /// filesystem metadata / junk directories, or is cyclic/broken.
    pub fn path(&self, idx: u32) -> Option<String> {
        let mut segments: Vec<u32> = Vec::with_capacity(16);
        let mut cur = idx;
        for _ in 0..128 {
            segments.push(cur);
            let parent = self.entries[cur as usize].parent;
            if parent == NO_PARENT {
                let mut path = String::with_capacity(64);
                for (i, seg) in segments.iter().rev().enumerate() {
                    let name = self.name(*seg);
                    if i > 0 {
                        if is_junk_component(name) {
                            return None;
                        }
                        path.push('\\');
                    }
                    path.push_str(name);
                }
                return Some(path);
            }
            cur = parent;
        }
        None // depth cap exceeded: broken parent chain
    }
}

fn is_junk_component(name: &str) -> bool {
    name.starts_with('$') || name.eq_ignore_ascii_case("System Volume Information")
}

pub struct SearchHit {
    pub idx: u32,
    pub score: u32,
}

/// Parallel fuzzy match over all entry names. Each rayon chunk keeps its own
/// Matcher and UTF-32 buffers; results are merged and truncated to `limit`.
pub fn search(index: &FileIndex, query: &str, limit: usize) -> Vec<SearchHit> {
    if query.is_empty() || index.len() == 0 {
        return Vec::new();
    }

    let pattern = Pattern::parse(query, CaseMatching::Ignore, Normalization::Smart);
    let mut config = Config::DEFAULT;
    config.prefer_prefix = true;

    const CHUNK: usize = 16 * 1024;
    let n = index.len();
    let chunk_count = (n + CHUNK - 1) / CHUNK;

    let mut hits: Vec<SearchHit> = (0..chunk_count)
        .into_par_iter()
        .map(|c| {
            let lo = c * CHUNK;
            let hi = (lo + CHUNK).min(n);
            let mut matcher = Matcher::new(config.clone());
            let mut buf: Vec<char> = Vec::with_capacity(256);
            let mut local: Vec<SearchHit> = Vec::new();
            for i in lo..hi {
                let name = index.name(i as u32);
                let haystack = Utf32Str::new(name, &mut buf);
                if let Some(score) = pattern.score(haystack, &mut matcher) {
                    local.push(SearchHit {
                        idx: i as u32,
                        score,
                    });
                }
            }
            // Keep chunks bounded so the merge stays cheap on broad queries.
            if local.len() > limit * 4 {
                local.sort_unstable_by(|a, b| b.score.cmp(&a.score));
                local.truncate(limit * 4);
            }
            local
        })
        .reduce(Vec::new, |mut a, mut b| {
            a.append(&mut b);
            a
        });

    hits.sort_unstable_by(|a, b| b.score.cmp(&a.score));
    hits.truncate(limit * 4); // headroom for junk-path filtering + frecency re-rank
    hits
}
