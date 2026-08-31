//! Parallel directory-walk indexer: the non-elevated fallback when raw volume
//! access (USN enumeration) is unavailable. Walks configured roots with rayon,
//! one task per top-level subdirectory.

use std::collections::HashSet;
use std::fs;
use std::path::{Path, PathBuf};

use rayon::prelude::*;

use crate::engine::{FileIndex, IndexBuilder, NO_PARENT};

const FILE_ATTRIBUTE_REPARSE_POINT: u32 = 0x400;

pub const DEFAULT_SKIP_DIRS: &[&str] = &[
    ".git",
    "node_modules",
    "__pycache__",
    ".cache",
    "AppData",
    "$Recycle.Bin",
    "System Volume Information",
    "Windows.old",
];

struct LocalEntry {
    name: String,
    parent: u32, // local index within the subtree; entry 0 is the subtree root
    is_dir: bool,
}

fn should_skip(name: &str, skip: &HashSet<String>) -> bool {
    skip.contains(&name.to_ascii_lowercase())
}

fn is_reparse_point(entry: &fs::DirEntry) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        entry
            .metadata()
            .map(|m| m.file_attributes() & FILE_ATTRIBUTE_REPARSE_POINT != 0)
            .unwrap_or(true)
    }
    #[cfg(not(windows))]
    {
        entry
            .file_type()
            .map(|t| t.is_symlink())
            .unwrap_or(true)
    }
}

/// Iterative walk of one subtree into local (name, parent) records.
fn walk_subtree(root: &Path, skip: &HashSet<String>) -> Vec<LocalEntry> {
    let root_name = root
        .file_name()
        .map(|n| n.to_string_lossy().into_owned())
        .unwrap_or_default();
    let mut out = vec![LocalEntry {
        name: root_name,
        parent: NO_PARENT,
        is_dir: true,
    }];

    let mut stack: Vec<(PathBuf, u32)> = vec![(root.to_path_buf(), 0)];
    while let Some((dir, parent_idx)) = stack.pop() {
        let Ok(read) = fs::read_dir(&dir) else {
            continue;
        };
        for entry in read.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            let is_dir = entry.file_type().map(|t| t.is_dir()).unwrap_or(false);
            let idx = out.len() as u32;
            out.push(LocalEntry {
                name,
                parent: parent_idx,
                is_dir,
            });
            if is_dir
                && !should_skip(&out[idx as usize].name, skip)
                && !is_reparse_point(&entry)
            {
                stack.push((entry.path(), idx));
            }
        }
    }
    out
}

fn merge_subtree(builder: &mut IndexBuilder, subtree: Vec<LocalEntry>, root_parent: u32) {
    let base = builder.len() as u32;
    for entry in subtree {
        let parent = if entry.parent == NO_PARENT {
            root_parent
        } else {
            base + entry.parent
        };
        builder.add(&entry.name, parent, entry.is_dir);
    }
}

/// Index the given roots. Each root becomes a top-level entry whose name is the
/// full root path, so resolved paths come out absolute.
pub fn build(roots: &[PathBuf], skip_dirs: &[String]) -> FileIndex {
    let skip: HashSet<String> = skip_dirs.iter().map(|s| s.to_ascii_lowercase()).collect();
    let mut builder = IndexBuilder::new();

    for root in roots {
        let root_name = root.to_string_lossy();
        let root_name = root_name.trim_end_matches('\\');
        let root_idx = builder.add(root_name, NO_PARENT, true);

        // First level sequentially; fan out one rayon task per subdirectory.
        let Ok(read) = fs::read_dir(root) else {
            continue;
        };
        let mut subdirs: Vec<PathBuf> = Vec::new();
        for entry in read.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            let is_dir = entry.file_type().map(|t| t.is_dir()).unwrap_or(false);
            if is_dir {
                if !should_skip(&name, &skip) && !is_reparse_point(&entry) {
                    subdirs.push(entry.path());
                } else {
                    builder.add(&name, root_idx, true);
                }
            } else {
                builder.add(&name, root_idx, false);
            }
        }

        let subtrees: Vec<Vec<LocalEntry>> = subdirs
            .par_iter()
            .map(|dir| walk_subtree(dir, &skip))
            .collect();
        for subtree in subtrees {
            merge_subtree(&mut builder, subtree, root_idx);
        }
    }

    builder.finish()
}
