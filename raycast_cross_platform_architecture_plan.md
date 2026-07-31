# Strategic Technical Blueprint: Building a Cross-Platform Raycast-Like Launcher for Windows & Linux

---

## 1. System Architecture & Tech Stack Selection

To achieve the performance, responsiveness, and aesthetic polish expected of a desktop launcher across diverse environments (Windows 11, Hyprland, GNOME, KDE Plasma 6, COSMIC), we evaluate two primary architectural paradigms:

```
                          ┌──────────────────────────────────────┐
                          │  Raycast Extensions (React + TS)     │
                          │  Using official @raycast/api         │
                          └──────────────────┬───────────────────┘
                                             │ JSON-RPC IPC
                                ┌────────────┴───────────┐
                                │   Node.js Extension    │
                                │   Host & Reconciler    │
                                └────────────┬───────────┘
                                             │ IPC / Shared Memory
 ┌───────────────────────────────────────────┴───────────────────────────────────────────┐
 │                                   NATIVE LAUNCHER ENGINE                               │
 │                                                                                       │
 │   ┌─────────────────────────────────────┐     ┌───────────────────────────────────┐   │
 │   │        Linux Desktop Shell          │     │        Windows OS Shell           │   │
 │   │  - Wayland Layer-Shell (Hypr/KDE)   │     │  - Win32 / DWM Mica/Acrylic       │   │
 │   │  - XDG Global Shortcuts Portal      │     │  - RegisterHotKey API             │   │
 │   │  - Freedesktop .desktop Parser      │     │  - Start Menu & AppX Indexer      │   │
 │   └─────────────────────────────────────┘     └───────────────────────────────────┘   │
 │                                                                                       │
 │   ┌───────────────────────────────────────────────────────────────────────────────┐   │
 │   │                           Presentation Layer                                  │   │
 │   │   Option A: Qt 6 Quick / QML (Recommended for Linux/Windows hybrid)           │   │
 │   │   Option B: Custom Rust Core + System Webview / GTK4                            │   │
 │   └───────────────────────────────────────────────────────────────────────────────┘   │
 │                                                                                       │
 │   ┌───────────────────────────────────────────────────────────────────────────────┐   │
 │   │                     Rust Background Engine (Shared Services)                  │   │
 │   │   - Nucleo Fuzzy Search Engine                                                │   │
 │   │   - NTFS MFT Direct Drive Indexer (Windows) / Inotify Scanner (Linux)         │   │
 │   │   - Local SQLite Data Store & Snippet Engine                                  │   │
 │   └───────────────────────────────────────────────────────────────────────────────┘   │
 └───────────────────────────────────────────────────────────────────────────────────────┘
```

### Stack Comparison

| Strategy | Presentation Layer | RAM Footprint | Cold Start | Wayland / Win32 Parity | Verdict |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Option A (Vicinae Model)** | **Qt 6 / QML + C++23** | **~50–90 MB** | **< 30 ms** | **Native** via `qt-layer-shell` / Win32 | **Recommended** (Best performance & low RAM) |
| **Option B (Raycast 2.0 Model)**| **React + Webview + Rust Shell**| **~300–400 MB**| **~150 ms**| Requires `gtk-layer-shell` / WebView2 | Higher UI iteration speed, higher RAM cost |
| **Option C** | **Rust + Slint / iced** | **~30–50 MB** | **< 20 ms** | Requires custom Wayland bindings | Immature ecosystem for complex UI controls |

---

## 2. Raycast Extension Ecosystem Compatibility Engine

To seamlessly execute Raycast extensions compiled from the official [Raycast Store](https://www.raycast.com/store), the system must intercept and implement the **`@raycast/api`** protocol.

### A. The Node.js Extension Host
1. **Isolated Worker Process**: Run a single long-lived Node.js worker per extension execution (or a managed pool).
2. **React Reconciler / AST Bridge**:
   - Raycast extensions render standard declarative React components: `<List>`, `<Form>`, `<Detail>`, `<Grid>`, `<ActionPanel>`, `<Action>`, `<Icon>`.
   - Create a custom React Reconciler for `@raycast/api` that serializes the React tree into JSON-RPC messages (e.g., `render_view`, `update_items`, `push_view`, `show_toast`, `copy_to_clipboard`).
3. **API Environment Shims**:
   - Shim Raycast runtime globals: `environment`, `Preferences`, `LocalStorage` (backed by SQLite/KV store), `Clipboard`, `showToast()`, `open()`, `trash()`.

### B. IPC Message Flow Example
```
Extension (React) ──► Custom Reconciler ──► JSON-RPC over stdio/socket ──► Native Launcher (Qt/Rust) ──► Native QML/Web View Render
```

---

## 3. Linux Compositor & Desktop Environment Integration

Linux presents unique challenges due to fragmentation across display servers (Wayland vs. X11) and compositors.

### A. Wayland Integration Strategy
1. **Hyprland, KDE Plasma 6, Sway, COSMIC (wlroots / Smithay)**:
   - Use the **Wayland Layer-Shell Protocol (`zwlr_layer_shell_v1`)** via `gtk-layer-shell` or `qt-layer-shell`.
   - Set Layer to `Overlay`, keyboard focus mode to `Exclusive` or `OnDemand`, and anchor center.
2. **GNOME Shell (Mutter)**:
   - GNOME does **not** support `zwlr_layer_shell_v1`.
   - Workaround: Use standard `xdg_toplevel` with custom frameless window hints + `XDG_POSITIONER`, or ship an optional thin GNOME Shell Extension companion to handle front-ordering and focus grabbing.
3. **Global Hotkeys under Wayland**:
   - Use the **XDG Desktop Portal Global Shortcuts API** (`org.freedesktop.portal.GlobalShortcuts`) to allow users to configure global hotkeys without security/compositor restrictions.
   - For Hyprland specifically: Expose IPC binding helpers (`hyprctl keyword bind ...`).

### B. Desktop App Indexer (`.desktop` Files)
- Parse Freedesktop standard desktop files from:
  - `/usr/share/applications`
  - `~/.local/share/applications`
  - `/var/lib/flatpak/exports/share/applications`
  - `/run/current-system/sw/share/applications` (NixOS)
- Cache icon paths (`/usr/share/icons`, hicolor themes, SVG/PNG icons) and execute command lines (`Exec=...`).

---

## 4. Windows OS Integration Strategy

### A. Window Management & Aesthetics
- **Frameless Overlay Window**: Create a borderless Win32 window (`WS_POPUP`).
- **Mica & Acrylic Visual Effects**: Apply Windows 11 Fluent materials using `SetWindowCompositionAttribute` or `DwmSetWindowAttribute` (`DWMWA_SYSTEMBACKDROP_TYPE`).
- **Focus & Focus Theft Prevention**: Handle `WM_KILLFOCUS` to auto-dismiss when focus changes. When executing paste/snippet insertion into active apps, use `AttachThreadInput` / `SendInput` to return focus to the previous active window smoothly.

### B. Global Hotkeys & System Tray
- Register global hotkey via Win32 `RegisterHotKey(hWnd, ID, MOD_ALT | MOD_NOREPEAT, VK_SPACE)`.
- System tray icon via `Shell_NotifyIconW`.

### C. Fast Application & File Indexing
- **Start Menu Indexer**: Walk `%APPDATA%\Microsoft\Windows\Start Menu\Programs` and `%PROGRAMDATA%\Microsoft\Windows\Start Menu\Programs` (`.lnk` files).
- **UWP / AppX Apps**: Query WinRT `PackageManager` to discover installed Store apps.
- **Fast NTFS Search**: Parse the NTFS **Master File Table (MFT)** in Rust for drive indexing in under 3 seconds.

---

## 5. Core Native Launcher Features Matrix

| Feature | Description | Implementation Strategy |
| :--- | :--- | :--- |
| **Root Search & Fuzzy Engine** | Sub-millisecond fuzzy ranking across apps, files, snippets, and commands | Rust `nucleo` or custom C++ fuzzy trait system |
| **Clipboard Manager** | Encrypted history of text, images, and HTML clips | SQLite database + native clipboard monitoring APIs |
| **Text Expander (Snippets)** | Keyword expansion with dynamic tags (`{clipboard}`, `{date}`) | Low-level keyboard hook (Win32 `WH_KEYBOARD_LL` / Linux `uinput` / `evdev` / X11 record) |
| **Calculator Engine** | Instant evaluation of math, units, currencies | Rust `libqalculate` bindings or `evalexpr` |
| **Browser Extension Bridge** | Tab search & active URL switching | Native Messaging Host extension protocol (Chrome/Firefox WebSocket/IPC) |

---

## 6. Phased Development Roadmap

### Phase 1: Core Engine & Node Extension Runtime (Weeks 1–4)
- Set up C++/Qt or Rust host shell.
- Implement Node.js worker IPC process and custom `@raycast/api` shims for `<List>` and `<Detail>` rendering.
- Implement fuzzy search matching engine in Rust.

### Phase 2: Linux & Windows Window Shell (Weeks 5–8)
- Build Wayland layer-shell integration for Hyprland/KDE/COSMIC and GNOME popup fallbacks.
- Build Win32 borderless acrylic window with hotkey registration.
- Freedesktop `.desktop` parser and Windows Start Menu `.lnk` parser.

### Phase 3: Extension Component Expansion & Store Sync (Weeks 9–12)
- Implement `<Form>`, `<Grid>`, `<ActionPanel>`, `<Toast>`, and `LocalStorage`.
- Build Extension Store downloader & installer capable of running Raycast open-source extensions.

### Phase 4: Native Modules & Polish (Weeks 13+)
- Add Clipboard Manager, Snippet Expander, Calculator, and File Indexer.
- Optimize memory and startup latency.
