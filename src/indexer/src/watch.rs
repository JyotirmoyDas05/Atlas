//! Filesystem change detection. Watchers only *signal* — the coordinator in
//! lib.rs debounces signals and triggers a full re-index (fast enough on both
//! paths: ~1s USN enumeration elevated, warm-cache walk otherwise).
//!
//! Elevated:     one thread per NTFS volume blocking on FSCTL_READ_USN_JOURNAL.
//! Non-elevated: one thread per configured root blocking on
//!               ReadDirectoryChangesW (subtree, rename/create/delete filter).

#![cfg(windows)]

use std::ffi::c_void;
use std::path::PathBuf;
use std::sync::mpsc::Sender;
use std::time::Duration;

use windows::core::PCWSTR;
use windows::Win32::Foundation::BOOL;
use windows::Win32::Storage::FileSystem::{
    CreateFileW, ReadDirectoryChangesW, FILE_FLAG_BACKUP_SEMANTICS, FILE_LIST_DIRECTORY,
    FILE_NOTIFY_CHANGE_DIR_NAME, FILE_NOTIFY_CHANGE_FILE_NAME, FILE_SHARE_DELETE,
    FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING,
};
use windows::Win32::System::Ioctl::{
    FSCTL_QUERY_USN_JOURNAL, FSCTL_READ_USN_JOURNAL, READ_USN_JOURNAL_DATA_V0,
    USN_JOURNAL_DATA_V0,
};
use windows::Win32::System::IO::DeviceIoControl;

use crate::usn;

fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(std::iter::once(0)).collect()
}

/// Blocks on the volume's USN journal; sends one signal per wakeup.
fn watch_usn_journal(letter: char, tx: Sender<()>) {
    loop {
        let Some(volume) = usn::open_volume(letter) else {
            return; // lost elevation / volume gone; coordinator keeps last index
        };

        let mut journal = USN_JOURNAL_DATA_V0::default();
        let mut bytes = 0u32;
        let queried = unsafe {
            DeviceIoControl(
                volume.0,
                FSCTL_QUERY_USN_JOURNAL,
                None,
                0,
                Some(&mut journal as *mut _ as *mut c_void),
                std::mem::size_of::<USN_JOURNAL_DATA_V0>() as u32,
                Some(&mut bytes),
                None,
            )
        };
        if queried.is_err() {
            std::thread::sleep(Duration::from_secs(60));
            continue;
        }

        let mut read_data = READ_USN_JOURNAL_DATA_V0 {
            StartUsn: journal.NextUsn,
            ReasonMask: 0xFFFF_FFFF,
            ReturnOnlyOnClose: 0,
            Timeout: 0,
            BytesToWaitFor: 1, // block until the journal has new data
            UsnJournalID: journal.UsnJournalID,
        };
        let mut buf = vec![0u8; 64 * 1024];

        loop {
            let mut returned = 0u32;
            let ok = unsafe {
                DeviceIoControl(
                    volume.0,
                    FSCTL_READ_USN_JOURNAL,
                    Some(&read_data as *const _ as *const c_void),
                    std::mem::size_of::<READ_USN_JOURNAL_DATA_V0>() as u32,
                    Some(buf.as_mut_ptr() as *mut c_void),
                    buf.len() as u32,
                    Some(&mut returned),
                    None,
                )
            };
            if ok.is_err() {
                break; // journal wrapped/deleted: re-query from the outer loop
            }
            if returned >= 8 {
                read_data.StartUsn = i64::from_le_bytes(buf[..8].try_into().unwrap());
            }
            if tx.send(()).is_err() {
                return;
            }
        }
    }
}

/// Blocks on ReadDirectoryChangesW for one root (subtree); one signal per batch.
fn watch_directory(root: PathBuf, tx: Sender<()>) {
    let path = wide(&root.to_string_lossy());
    let handle = unsafe {
        CreateFileW(
            PCWSTR(path.as_ptr()),
            FILE_LIST_DIRECTORY.0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            None,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            None,
        )
    };
    let Ok(handle) = handle else { return };
    let handle = usn::VolumeHandle(handle); // reuse RAII close

    let mut buf = vec![0u8; 64 * 1024];
    loop {
        let mut returned = 0u32;
        let ok = unsafe {
            ReadDirectoryChangesW(
                handle.0,
                buf.as_mut_ptr() as *mut c_void,
                buf.len() as u32,
                BOOL::from(true),
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
                Some(&mut returned),
                None,
                None,
            )
        };
        if ok.is_err() {
            return;
        }
        if tx.send(()).is_err() {
            return;
        }
    }
}

/// Spawn the appropriate watcher threads. Returns the mode for status logging.
pub fn spawn(tx: Sender<()>, roots: &[PathBuf]) -> &'static str {
    // Probe elevation by trying to open one volume handle.
    let volumes = usn::ntfs_volumes();
    let elevated = volumes.iter().any(|&l| usn::open_volume(l).is_some());

    if elevated {
        for letter in volumes {
            let tx = tx.clone();
            std::thread::spawn(move || watch_usn_journal(letter, tx));
        }
        "usn-journal"
    } else if !roots.is_empty() {
        for root in roots {
            let tx = tx.clone();
            let root = root.clone();
            std::thread::spawn(move || watch_directory(root, tx));
        }
        "rdcw"
    } else {
        "none"
    }
}
