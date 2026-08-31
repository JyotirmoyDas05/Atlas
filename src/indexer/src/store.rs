//! On-disk persistence: binary index snapshot (instant warm start) and the
//! frecency store used to boost recently/frequently opened results.

use std::collections::HashMap;
use std::fs::{self, File};
use std::io::{self, BufReader, BufWriter, Read, Write};
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use serde::{Deserialize, Serialize};

use crate::engine::{Entry, FileIndex};

const MAGIC: &[u8; 8] = b"ATLASIX1";

fn write_u32(w: &mut impl Write, v: u32) -> io::Result<()> {
    w.write_all(&v.to_le_bytes())
}

fn write_u64(w: &mut impl Write, v: u64) -> io::Result<()> {
    w.write_all(&v.to_le_bytes())
}

fn read_u32(r: &mut impl Read) -> io::Result<u32> {
    let mut b = [0u8; 4];
    r.read_exact(&mut b)?;
    Ok(u32::from_le_bytes(b))
}

fn read_u64(r: &mut impl Read) -> io::Result<u64> {
    let mut b = [0u8; 8];
    r.read_exact(&mut b)?;
    Ok(u64::from_le_bytes(b))
}

pub fn snapshot_path(data_dir: &Path) -> PathBuf {
    data_dir.join("file_index.bin")
}

pub fn save_snapshot(data_dir: &Path, index: &FileIndex) -> io::Result<()> {
    fs::create_dir_all(data_dir)?;
    let final_path = snapshot_path(data_dir);
    let tmp_path = final_path.with_extension("bin.tmp");
    {
        let mut w = BufWriter::new(File::create(&tmp_path)?);
        let (names, entries) = index.parts();
        w.write_all(MAGIC)?;
        write_u32(&mut w, entries.len() as u32)?;
        write_u64(&mut w, names.len() as u64)?;
        w.write_all(names.as_bytes())?;
        for e in entries {
            write_u32(&mut w, e.name_start)?;
            write_u32(&mut w, e.name_len as u32 | ((e.is_dir as u32) << 31))?;
            write_u32(&mut w, e.parent)?;
        }
        w.flush()?;
    }
    let _ = fs::remove_file(&final_path);
    fs::rename(&tmp_path, &final_path)
}

pub fn load_snapshot(data_dir: &Path) -> io::Result<FileIndex> {
    let mut r = BufReader::new(File::open(snapshot_path(data_dir))?);
    let mut magic = [0u8; 8];
    r.read_exact(&mut magic)?;
    if &magic != MAGIC {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "bad magic"));
    }
    let entry_count = read_u32(&mut r)? as usize;
    let names_len = read_u64(&mut r)? as usize;

    let mut names_bytes = vec![0u8; names_len];
    r.read_exact(&mut names_bytes)?;
    let names = String::from_utf8(names_bytes)
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "bad utf8 arena"))?;

    let mut entries = Vec::with_capacity(entry_count);
    for _ in 0..entry_count {
        let name_start = read_u32(&mut r)?;
        let packed = read_u32(&mut r)?;
        let parent = read_u32(&mut r)?;
        let name_len = (packed & 0xFFFF) as u16;
        let is_dir = packed >> 31 != 0;
        if name_start as usize + name_len as usize > names.len() {
            return Err(io::Error::new(io::ErrorKind::InvalidData, "entry out of arena"));
        }
        entries.push(Entry {
            name_start,
            name_len,
            is_dir,
            parent,
        });
    }
    Ok(FileIndex::from_parts(names, entries))
}

// ---------------------------------------------------------------------------
// Frecency
// ---------------------------------------------------------------------------

#[derive(Serialize, Deserialize, Clone, Copy)]
struct FrecencyEntry {
    count: u32,
    last_open: i64, // unix seconds
}

#[derive(Default)]
pub struct Frecency {
    map: HashMap<String, FrecencyEntry>,
}

fn frecency_path(data_dir: &Path) -> PathBuf {
    data_dir.join("frecency.json")
}

fn now_unix() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs() as i64)
        .unwrap_or(0)
}

impl Frecency {
    pub fn load(data_dir: &Path) -> Self {
        let map = fs::read(frecency_path(data_dir))
            .ok()
            .and_then(|bytes| serde_json::from_slice(&bytes).ok())
            .unwrap_or_default();
        Self { map }
    }

    pub fn save(&self, data_dir: &Path) {
        if fs::create_dir_all(data_dir).is_err() {
            return;
        }
        if let Ok(json) = serde_json::to_vec(&self.map) {
            let _ = fs::write(frecency_path(data_dir), json);
        }
    }

    pub fn record(&mut self, path: &str) {
        let entry = self
            .map
            .entry(path.to_ascii_lowercase())
            .or_insert(FrecencyEntry {
                count: 0,
                last_open: 0,
            });
        entry.count = entry.count.saturating_add(1);
        entry.last_open = now_unix();
    }

    /// Additive score bonus. Nucleo scores land in the low hundreds, so a
    /// frequently+recently opened file reliably outranks a cold better-match.
    pub fn bonus(&self, path: &str) -> u32 {
        let Some(e) = self.map.get(&path.to_ascii_lowercase()) else {
            return 0;
        };
        let freq = e.count.min(20) * 8;
        let age_secs = (now_unix() - e.last_open).max(0) as u64;
        let recency = match age_secs {
            0..=86_400 => 80,          // opened within a day
            ..=604_800 => 40,          // within a week
            ..=2_592_000 => 15,        // within a month
            _ => 0,
        };
        freq + recency
    }
}
