//! NTFS MFT enumeration via the USN journal (`FSCTL_ENUM_USN_DATA`).
//!
//! Reads every MFT record of a volume as one sequential stream instead of
//! walking the directory tree — this is what makes full-drive indexing take
//! ~1s instead of minutes. Requires a volume handle (`\\.\C:`), which needs
//! elevation; callers fall back to the directory walker when it fails.

#![cfg(windows)]

use std::collections::HashMap;
use std::ffi::c_void;

use windows::core::PCWSTR;
use windows::Win32::Foundation::{CloseHandle, GENERIC_READ, HANDLE};
use windows::Win32::Storage::FileSystem::{
    CreateFileW, GetDriveTypeW, GetLogicalDrives, GetVolumeInformationW,
    FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::Win32::System::Ioctl::{FSCTL_ENUM_USN_DATA, MFT_ENUM_DATA_V0};
use windows::Win32::System::IO::DeviceIoControl;

use crate::engine::{FileIndex, IndexBuilder, NO_PARENT};

const DRIVE_FIXED: u32 = 3;
const FILE_ATTRIBUTE_DIRECTORY: u32 = 0x10;

fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(std::iter::once(0)).collect()
}

/// Fixed NTFS drive letters, e.g. ['C', 'D'].
pub fn ntfs_volumes() -> Vec<char> {
    let mut out = Vec::new();
    let mask = unsafe { GetLogicalDrives() };
    for i in 0..26u32 {
        if mask & (1 << i) == 0 {
            continue;
        }
        let letter = (b'A' + i as u8) as char;
        let root = wide(&format!("{letter}:\\"));
        let drive_type = unsafe { GetDriveTypeW(PCWSTR(root.as_ptr())) };
        if drive_type != DRIVE_FIXED {
            continue;
        }
        let mut fs_name = [0u16; 32];
        let ok = unsafe {
            GetVolumeInformationW(
                PCWSTR(root.as_ptr()),
                None,
                None,
                None,
                None,
                Some(&mut fs_name),
            )
        };
        if ok.is_ok() {
            let fs = String::from_utf16_lossy(&fs_name);
            if fs.trim_end_matches('\0') == "NTFS" {
                out.push(letter);
            }
        }
    }
    out
}

pub(crate) struct VolumeHandle(pub(crate) HANDLE);

impl Drop for VolumeHandle {
    fn drop(&mut self) {
        unsafe {
            let _ = CloseHandle(self.0);
        }
    }
}

pub(crate) fn open_volume(letter: char) -> Option<VolumeHandle> {
    let path = wide(&format!("\\\\.\\{letter}:"));
    let handle = unsafe {
        CreateFileW(
            PCWSTR(path.as_ptr()),
            GENERIC_READ.0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            None,
            OPEN_EXISTING,
            FILE_FLAGS_AND_ATTRIBUTES(0),
            None,
        )
    };
    handle.ok().map(VolumeHandle)
}

fn read_u16(buf: &[u8], off: usize) -> u16 {
    u16::from_le_bytes([buf[off], buf[off + 1]])
}

fn read_u32(buf: &[u8], off: usize) -> u32 {
    u32::from_le_bytes(buf[off..off + 4].try_into().unwrap())
}

fn read_u64(buf: &[u8], off: usize) -> u64 {
    u64::from_le_bytes(buf[off..off + 8].try_into().unwrap())
}

/// Enumerate one volume's MFT into `builder`. Records are added with a
/// placeholder parent; the caller fixes parents up via the returned FRN map.
fn enumerate_volume(
    letter: char,
    builder: &mut IndexBuilder,
    frn_to_idx: &mut HashMap<u64, u32>,
    parent_frns: &mut Vec<(u32, u64)>,
) -> Result<u32, String> {
    let volume = open_volume(letter).ok_or_else(|| format!("cannot open \\\\.\\{letter}:"))?;

    let root_idx = builder.add(&format!("{letter}:"), NO_PARENT, true);

    let mut med = MFT_ENUM_DATA_V0 {
        StartFileReferenceNumber: 0,
        LowUsn: 0,
        HighUsn: i64::MAX,
    };
    let mut buf = vec![0u8; 1 << 20];

    loop {
        let mut bytes: u32 = 0;
        let ok = unsafe {
            DeviceIoControl(
                volume.0,
                FSCTL_ENUM_USN_DATA,
                Some(&med as *const _ as *const c_void),
                std::mem::size_of::<MFT_ENUM_DATA_V0>() as u32,
                Some(buf.as_mut_ptr() as *mut c_void),
                buf.len() as u32,
                Some(&mut bytes),
                None,
            )
        };
        if ok.is_err() || bytes < 8 {
            break; // ERROR_HANDLE_EOF: enumeration complete
        }

        med.StartFileReferenceNumber = read_u64(&buf, 0);

        let mut off = 8usize;
        while off + 60 <= bytes as usize {
            let rec_len = read_u32(&buf, off) as usize;
            if rec_len < 60 || off + rec_len > bytes as usize {
                break;
            }
            if read_u16(&buf, off + 4) == 2 {
                // USN_RECORD_V2
                let frn = read_u64(&buf, off + 8);
                let parent_frn = read_u64(&buf, off + 16);
                let attrs = read_u32(&buf, off + 52);
                let name_len = read_u16(&buf, off + 56) as usize;
                let name_off = read_u16(&buf, off + 58) as usize;

                if off + name_off + name_len <= bytes as usize {
                    let name_bytes = &buf[off + name_off..off + name_off + name_len];
                    let utf16: Vec<u16> = name_bytes
                        .chunks_exact(2)
                        .map(|c| u16::from_le_bytes([c[0], c[1]]))
                        .collect();
                    let name = String::from_utf16_lossy(&utf16);
                    if name == "." {
                        // The volume root's own MFT record parents itself; alias
                        // it to our root entry so children resolve correctly.
                        frn_to_idx.insert(frn, root_idx);
                    } else {
                        let is_dir = attrs & FILE_ATTRIBUTE_DIRECTORY != 0;
                        let idx = builder.add(&name, root_idx, is_dir);
                        frn_to_idx.insert(frn, idx);
                        parent_frns.push((idx, parent_frn));
                    }
                }
            }
            off += rec_len;
        }
    }

    Ok(root_idx)
}

/// Build a full index of every fixed NTFS volume. Err if no volume could be
/// opened (typically: not elevated).
pub fn build_all() -> Result<FileIndex, String> {
    let volumes = ntfs_volumes();
    if volumes.is_empty() {
        return Err("no fixed NTFS volumes found".into());
    }

    let mut builder = IndexBuilder::new();
    let mut opened = 0;

    for letter in volumes {
        let mut frn_to_idx: HashMap<u64, u32> = HashMap::new();
        let mut parent_frns: Vec<(u32, u64)> = Vec::new();

        match enumerate_volume(letter, &mut builder, &mut frn_to_idx, &mut parent_frns) {
            Ok(root_idx) => {
                opened += 1;
                for (idx, parent_frn) in parent_frns {
                    // Unknown parents (root FRN, orphans) attach to the volume root.
                    let mut parent = frn_to_idx.get(&parent_frn).copied().unwrap_or(root_idx);
                    if parent == idx {
                        parent = root_idx; // never self-parent: would cycle path resolution
                    }
                    builder.set_parent(idx, parent);
                }
            }
            Err(_) => continue,
        }
    }

    if opened == 0 {
        return Err("could not open any NTFS volume (requires elevation)".into());
    }
    Ok(builder.finish())
}
