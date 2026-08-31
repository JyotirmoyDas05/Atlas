# Vicinae: Deep Study & the Atlas Roadmap From Here

*Study of `D:\Code-Projects\vicinae` @ v0.27.5 (2026-08-31, GPL-3.0). Four deep-dive passes: core architecture, UI/QML system, extension runtime, features & Windows-port state.*

---

## 0. What Vicinae Is (and why it changes Atlas's plan)

Vicinae is a native command palette built on **exactly Atlas's stack — QtQuick/QML + C++23** — with full **Raycast extension compatibility** (React/TS extensions run unmodified, installed from Raycast's real store backend). It is what Atlas's original architecture plan describes, already built: ~950 C++ files, 133 QML files (~14.3k lines), ~25k lines of server C++, ~11.7k lines of TypeScript, shipped for Linux and macOS.

Two headline corrections to our prior assumptions:

1. **Vicinae's Windows port is ~85% done and production-grade** — global hotkey (LL hook + key-up suppression), PowerToys-style foreground grant, Win32+UWP app discovery via WinRT `PackageManager`, EnumWindows/WinEvent window switcher with registry-based virtual desktops, clipboard history honoring every Windows privacy format with AUMID source attribution, UI Automation selected-text, DWM acrylic chrome, `ms-settings:` provider, working Node extension runtime, Inno Setup installer built in CI. What's missing: text expansion, volume control, in-app autostart, self-update, code signing — **and they don't ship Windows binaries in releases at all**.
2. **It's GPL-3.0.** Copying any code into Atlas requires Atlas to be GPL-3.0 when distributed. Studying architecture/patterns and reimplementing is unrestricted. (Their `@vicinae/api` npm package's own license field should be checked separately — if permissive, the extension SDK could be depended on without copyleft reaching the binary.)

Development is very active: last commit the day of this study, 362 commits in 3 months.

---

## 1. Pros of Vicinae's Approach

- **Total validation of the native-QML thesis.** They deliver a Raycast-quality (arguably better-polished) UI with no WebView, low memory, and 60fps — proving Atlas's stack choice at production scale.
- **A tiny, disciplined view contract.** The entire C++↔QML bridge is a 27-line `ViewHostBase` (`qmlComponentUrl()` + `qmlProperties()` + lifecycle hooks); one `NavigationController` owns a logical view stack with **caller-scoped state mutators** (background views can't corrupt visible chrome); one QML `StackView` renders it. ~45 view hosts, zero per-view plumbing.
- **Composable list architecture.** `SectionSource` (59 lines) + `SectionListModel` flatten N pluggable sources into one sectioned model with selection preservation by item id. Root search, clipboard, emoji, apps, themes all reuse it; a new list view is ~40 lines (`MonoListViewHost<T>`).
- **Search done right.** Their fzf port separates **quality** (unweighted worst-word score, constant threshold, used to *filter*) from **score** (field-weighted, used to *rank*) plus a `coherent` flag rejecting nonsense alignments. Frecency is deliberately only 6 of 100 points — a tiebreaker, not a ranker. `stable_sort` prevents result flicker. A `FuzzySearchable<T>` trait makes every list fuzzy by default.
- **Service-oriented platform abstraction.** ~40 services behind `Abstract*` bases with per-OS backends chosen by `#ifdef` or activation-priority factories. This is how one codebase runs on Wayland/X11/macOS/Windows without hacks.
- **Own IPC IDL (`figura`, ~2.9k LOC)** generating type-safe JSON-RPC clients/servers in C++ *and* TypeScript from six small `.fig` schemas — one wire protocol shared by the CLI, file indexer, snippet server, and extension runtime. Windows transport: named pipe.
- **The Raycast-compat moat, solved.** Custom React reconciler (422 lines — the easy part) + `model-deser.cpp` (1,245 lines — the hard part: reverse-engineered knowledge of Raycast's undocumented polymorphic prop shapes). Node runtime auto-downloaded and SHA-256-verified; one process, one worker thread per command, pre-warmed pool; 3-step load handshake; 5s unload grace for React cleanup.
- **UI polish beyond Raycast:** frosted floating status bar with content scrolling underneath (`StatusBarInset`), hover inert until the mouse physically moves (`HoverActivation` — fixes "list re-sorts under stationary cursor"), SDF-shader rounded rects that blend correctly over translucent backgrounds (`SourceBlendRect`), quadratic `liftedOpacity` so selection fills stay legible at any transparency, live TOML theme hot-reload with 31 bundled themes + token derivation from ~6 core colors, cross-block markdown text selection, a11y annotations, i18n.

## 2. Cons / Risks of Vicinae's Approach

- **Multi-process sprawl** (7+ binaries: server, CLI, file indexer, extension manager, clipboard watcher, snippet/input servers, browser link). Justified at their maturity; overkill early.
- **Extension rendering has no wire diffing** — a dirty view re-serializes its whole subtree as JSON (triple-encoded through two RPC layers) at up to 60Hz. They mitigate (off-thread parse, native fuzzy filtering, debounce) but both sides carry perf instrumentation proving it hurt. A 2,000-item extension list is the failure mode.
- **Windows is the third platform.** File search delegates to Windows Search (WSearch OLE DB, `LIKE` recall + local fuzzy re-rank — same approach Atlas just *replaced* with its USN indexer); their excellent trigram/spellfix FTS5 indexer is Linux-only; snippets are evdev/uinput (unportable); qalculate is Linux/macOS; no self-update or signed installer on Windows.
- **GPL-3.0** constrains anyone wanting a permissive core or commercial dual-licensing.
- **No AI, no notes, no cloud sync** — the Raycast 2.0 suite features are absent (an AI branch exists).
- **They ship the org, not just an app**: store backends, npm packages, nix modules, translations — a lot of surface for a solo dev to mirror blindly.

---

## 3. Strategic Decision (needs owner sign-off)

Three honest options:

| Option | What it means | Verdict |
|---|---|---|
| **A. Adopt GPL-3 + copy freely** | License Atlas GPL-3.0, copy/port Vicinae code with attribution (UI shaders, model-deser, TS packages, theme files), keep Atlas's own USN indexer + Windows-first focus | **Recommended.** Fastest route to "looks and functions like Vicinae or better"; keeps everything legal; Atlas differentiates on Windows depth |
| B. Study-only, reimplement | Keep license freedom; rebuild every pattern from the reports without copying expression | 3–6× slower; the wire-shape knowledge in `model-deser.cpp` alone costs months to re-derive |
| C. Contribute to Vicinae upstream | Ship the missing Windows pieces (USN indexer, text expansion, WASAPI volume, self-update) as PRs | Highest leverage per hour, but it's their project — Atlas stops being a product |

**Recommendation: Option A**, with Atlas's identity = *the Windows-native one that goes deeper than Vicinae on Windows*: USN/MFT file search (already better than theirs), Windows text expansion (they have none), WASAPI volume, self-update, signed installer/winget. Every one of those is a gap they've left open.

---

## 4. The Roadmap From Here

Atlas today: fast Rust USN indexer + snapshot + watchers, working QML palette (apps/files/calc/clipboard/snippets), 9ms toggle, tray, single-instance, hotkey watchdog, perf harness. The gap to Vicinae is architecture (no view stack, no sections model, ad-hoc actions), UI depth, features, and extensions.

### Phase 3 — Vicinae-grade backbone (the big refactor)
*Everything else builds on this; do it before adding features.*

1. **License + attribution**: add GPL-3.0 `LICENSE`, `NOTICE` crediting Vicinae where code is ported.
2. **Navigation & view system**: port `NavigationController` (caller-scoped `ViewState`, deferred `deleteLater`), `ViewHostBase` (27-line contract), one `StackView` in the window shell. Refs: `src/server/src/navigation-controller.{hpp,cpp}`, `qml/bridge-view.hpp`, `qml/launcher-window.cpp:457`.
3. **Sections model**: port `SectionSource` / `SectionListModel` / `MonoListViewHost<T>` / `ViewScope`; rewrite root search as ordered sections (calculator → apps → snippets → files → fallbacks) with per-source async + staleness guards. Refs: `qml/section-source.hpp`, `section-list-model.cpp`, `root-search-model.cpp`.
4. **Fuzzy + ranking**: port `fzf.hpp` (quality/score split, `coherent`), `FuzzySearchable<T>` trait, frecency-as-tiebreaker; wire the Rust nucleo path to report compatible scores or adopt fzf in Rust for files too. Refs: `src/lib/fuzzy/`.
5. **Action panel**: port `ActionPanelState` + `finalize()` shortcut presets, `ActionPanelController`, submenu stack. Refs: `ui/action-pannel/`, `qml/qml/ActionListPanel.qml`.
6. **Command registry**: `AbstractCmd` / repositories / `EntrypointId` so builtins and future extensions are the same thing to root search. Refs: `command.hpp`, `command-database.cpp`, `single-view-command-context.hpp`.
7. **Config**: JSONC + `Partial<T>` delta-writeback + live reload + `(next, prev)` change signal. Refs: `config/config.{hpp,cpp}`.

### Phase 4 — Vicinae-grade UI shell
1. **Window shell**: rebuild `MainPalette.qml` on the `LauncherWindow.qml` recipe — masked shadow, `SourceBlendRect` border ring, DWM acrylic via their `WindowsWindow` attached-property pattern, compact mode, frosted floating footer + `StatusBarInset`.
2. **Core components** (the ranked-12 list): `GenericListView`, `ListItemDelegate`/`SelectableDelegate`/accessories/badges, `SearchBar` keyboard model (`matchNavigation` indirection, 16ms debounce, backspace-pops, emacs mode), `ViciScrollBar`, `HoverActivation` + reset helpers, `ViciPopover`/`PopoverBackground`.
3. **Theming**: port the ~40-token `SemanticColor` vocabulary, `theme-file.cpp` derivation table, TOML theme parsing, hot-reload; copy the 31 bundled themes (GPL, attribution). Ref: `theme/`, `extra/themes/`.
4. **Icons**: `ImageURL` typed model + `Img` QML factory + mask-tint SVG recoloring + the 853-icon builtin set (verify icon licensing — many derive from Lucide/Heroicons) + 3-tier image cache. Refs: `ui/image/`, `qml/vici-image-item.cpp`.
5. **Markdown**: block-model architecture over vendored cmark-gfm for detail views. Refs: `qml/markdown/`.

### Phase 5 — Feature parity (each ~days with the backbone in place)
Ranked by value ÷ cost from the study:
1. Root-item aliases, favorites, per-command hotkeys, fallback commands (`RootItemManager`, 699L).
2. Clipboard history v2: SQLite(+SQLCipher) with on-disk AES-GCM blobs, Windows privacy formats, AUMID attribution, pins/keywords. Refs: `services/clipboard/windows/`, `clipboard-formats.hpp`.
3. Window switcher: EnumWindows + WinEvent hook cache + registry virtual desktops. Refs: `window-manager/windows/`.
4. App discovery upgrade: add the UWP/WinRT `PackageManager` track + `App Paths` to Atlas's Start-Menu scan. Ref: `app-service/windows/win-app-database.cpp` (1,065L).
5. Emoji/symbol picker: generated static dataset + `GenericGridView`. Refs: `lib/glyph/`.
6. Calculator upgrade: replace QJSEngine with a real engine (their `numen` is a separate versioned dep — check its license; else soulver-style parsing or exprtk) + currency rates.
7. Settings window: pages + `ShortcutRecorderField` + preference forms. Refs: `settings-controller/`, `qml/qml/*SettingsPage.qml`.
8. Quicklinks, `ms-settings:` provider, system commands — near-free wins.

### Phase 6 — Extension runtime (the moat)
Adopt, don't rewrite: vendor `src/typescript/{api,raycast-api-compat,extension-manager}` (GPL path) or depend on the npm `@vicinae/api` if its license allows; implement the 11 `tsapi.fig` services in Atlas (JSON-RPC over stdio, 4-byte LE length framing); **port `model-deser.cpp` deliberately** — it is the irreplaceable file; build `ExtensionViewHost` + per-type models on the Phase 3 backbone. Node runtime: copy the download/verify/provision pattern. Est. 2–3 months vs 6–9 from scratch.

### Phase 7 — Beat Vicinae on Windows (the differentiators)
1. **USN indexer** — already built; polish + elevation service story (they have nothing like it).
2. **Windows text expansion** — LL-hook typed-buffer tracker + SendInput replacement (their snippet engine is unportable Linux; this is the single biggest hole in their Windows story).
3. **WASAPI volume control**, **in-app autostart** (done), **self-update** (GitHub Releases poll + installer swap), **signed installer + winget**.
4. Ship actual Windows releases — which Vicinae still doesn't.

---

## 5. What NOT to copy (at this stage)

- The 7-binary process split — Atlas stays 1 process + Rust-in-proc + (later) 1 Node process until something forces otherwise.
- `figura` — a plain JSON-RPC library + hand-written TS types covers Atlas's 1–2 boundaries; revisit if schemas multiply.
- The whole-subtree extension render model — if/when we build the host, budget for wire diffing from day one (our chance to beat them).
- Browser extension, dmenu/CLI, store backend, nix/i18n infrastructure — later, if ever.
- Their Windows file search (WSearch OLE DB) — ours is already better; keep WSearch only as the pre-index fallback (already the case).
