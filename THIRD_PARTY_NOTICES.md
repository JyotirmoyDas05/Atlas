# Third-Party Notices

Atlas is licensed under the GNU General Public License v3.0 (see `LICENSE`).
Some source files adapt designs and, in places, code from other GPL-3.0
projects. Per-file notices are kept at the top of each affected file; this
document is an index of them.

## Vicinae

[Vicinae](https://github.com/vicinaehq/vicinae) — a native command palette
for Linux/macOS, GPL-3.0, Copyright (C) Vicinae contributors.

Files below adapt Vicinae's architecture (in some cases directly translating
class/method structure into Atlas's naming and simplified for Atlas's current
feature set, which lacks Vicinae's `ApplicationContext`/`ImageURL` machinery):

- `src/core/actions/Action.hpp` — adapted from
  `src/server/src/ui/action-pannel/action.hpp`
- `src/core/actions/ActionPanelState.hpp` — adapted from
  `src/server/src/ui/action-pannel/action-panel-state.hpp`
- `src/core/config/ConfigValue.hpp` — adapted from the `Partial<T>` /
  merge-over-defaults pattern in `src/server/src/config/config.hpp`

Each file carries its own header comment with this same attribution.
