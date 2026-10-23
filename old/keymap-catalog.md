# Keymap catalog (nvim + VSCode + Noni cross-check)

Facts only. `KEEP?` khali hai.

**Leader:** `vim.g.mapleader = " "` (`~/.config/nvim/init.lua:2`)
**Localleader:** `vim.g.maplocalleader = " "` (`~/.config/nvim/init.lua:3`)
**timeoutlen:** 300 (`lua/config/options.lua:27`)

Yeh setup **LazyVim distro nahi**. `init.lua` comment: "Simple LazyVim-inspired Neovim config". `lua/config/lazy.lua` sirf `{ import = "plugins" }` — LazyVim plugin import nahi.

**VSCodeVim / vscode-neovim:** installed nahi (`code --list-extensions` + `~/.vscode/extensions/` names). `settings.json` me `vim.*` keys nahi.

---

## Paths padhe gaye

| Path | Status |
|---|---|
| `~/.config/nvim/init.lua` | read |
| `~/.config/nvim/lazy-lock.json` | read |
| `~/.config/nvim/lua/config/keymaps.lua` | read |
| `~/.config/nvim/lua/config/options.lua` | read |
| `~/.config/nvim/lua/config/autocmds.lua` | read |
| `~/.config/nvim/lua/config/lazy.lua` | read |
| `~/.config/nvim/lua/plugins/coding.lua` | read |
| `~/.config/nvim/lua/plugins/editor.lua` | read |
| `~/.config/nvim/lua/plugins/ui.lua` | read |
| `~/.config/nvim/lua/theme.lua` | read (no keymaps) |
| `~/.local/share/nvim/lazy/` plugin trees (Comment, blink.cmp presets, nvim-surround plugin.lua) | read |
| `/tmp/nvim-maps.txt` + `/tmp/nvim-imaps.txt` (`nvim --headless` verbose map / map!) | read |
| `~/.config/Code/User/keybindings.json` | read |
| `~/.config/Code/User/settings.json` | read |
| `code --list-extensions` | ran |
| `~/.vscode/extensions/` names | listed |
| Noni `config.json` keybindings | read |
| Noni `sys.txt` PART T / U / W | read |

## NOT ACCESSIBLE

| Path | Note |
|---|---|
| `~/.config/nvim/lazyvim.json` | file exist nahi |
| `~/.local/share/nvim/lazy/LazyVim/lua/lazyvim/config/keymaps.lua` | dir exist nahi |
| `~/.local/share/nvim/lazy/LazyVim/lua/lazyvim/plugins/**` | dir exist nahi |
| `~/.var/app/com.visualstudio.code/config/Code/User/` | exist nahi |
| `~/.config/Code - Insiders/User/` | exist nahi |

**Isliye:** koi `lazyvim-default` row nahi. Koi `user-override` of LazyVim defaults nahi (LazyVim load hi nahi hota). Disabled LazyVim keys list empty.

---

## which-key groups (`lua/plugins/ui.lua` spec)

| Prefix | Group label |
|---|---|
| `<leader>f` | find |
| `<leader>l` | lsp |
| `<leader>g` | git |
| `<leader>b` | buffers |
| `<leader>x` | diagnostics |
| `<leader>t` | terminal |
| `<leader>u` | ui |
| `<leader>m` | macros |
| `<leader>c` | code |
| `<leader>s` | session |

`<leader>u` ke liye user lua me koi key nahi mili.

---

## nvim --headless dump vs files

Dump **global** maps. Miss: `LspAttach` buffer maps, `gitsigns` `on_attach`, `blink.cmp` (`InsertEnter`), `nvim-surround` (`VeryLazy` — dump me ys/ds/cs nahi), neo-tree `window.mappings`.

Dump **extra** (user lua me nahi, `vim/_core/defaults` / matchit): `grn` rename, `gra` code action, `grr` references, `gri` implementation, `grt` type def, `grx` codelens, `gO` document_symbol, `K` nahi (user LspAttach pe buffer `K`), `[d`/`]d` diagnostic jump, `[q`/`]q` qflist, `Y`→`y$`, snippet `<Tab>` insert/select, `<C-W>d` diagnostic float, matchit `%`.

Dump **overwrite fact:** `keymaps.lua` pehle `<A-Up>`/`<A-Down>` ko window resize map karta hai, phir same keys ko move-line. Dump me `<M-Up>`/`<M-Down>` = move line; `<M-Left>`/`<M-Right>` = resize.

---

# NVIM

Columns: Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP?

## Editing / Motions

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<C-s>` | n, i | Save file (`:w`) | user-custom (`keymaps.lua:4`) | bound: `workbench.action.files.save` (`when`: `editorFocus`) | yes |  |
| `<leader>w` | n | Save file | user-custom (`keymaps.lua:5`) | missing (save `ctrl+s` / `:w`) | yes |  |
| `jk` | i | `<Esc>` | user-custom (`keymaps.lua:39`) | hardcoded: `editor.cpp` insert `jj` (not `jk`) | yes |  |
| `<Esc>` | n | `:nohlsearch` | user-custom (`keymaps.lua:9`) | bound: `noni.mode.normal` (`when` empty) | yes |  |
| `<` / `>` | v | indent, reselect (`<gv` / `>gv`) | user-custom (`keymaps.lua:37-38`) | hardcoded: visual has no indent keys in sys.txt T3 | yes |  |
| `<A-j>` `<A-k>` | n, i, v | Move line/selection down/up | user-custom (`keymaps.lua:41-46`) | missing | no: from_raw Alt set nahi (sys.txt T2/W) |  |
| `<A-Down>` `<A-Up>` | n, v | Move line/selection (later mapping; dump yehi dikhata) | user-custom (`keymaps.lua:47-50`) | missing | no: Alt |  |
| `<leader>mr` | n | Toggle macro record `@a` | user-custom (`keymaps.lua:53-61`) | missing | yes |  |
| `<leader>ma` | n | Play macro `@a` | user-custom (`keymaps.lua:62`) | missing | yes |  |
| `<leader>ma` | v | `:norm @a` | user-custom (`keymaps.lua:63`) | missing | yes |  |

## Windows / Splits

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<C-h>` | n | `<C-w>h` Focus left | user-custom (`keymaps.lua:12`) | bound: `ctrl+k h` → `workbench.action.focusLeftGroup`; raw 8 = Backspace in from_raw | no: Ctrl+H = Backspace same byte (sys.txt T2) |  |
| `<C-j>` | n | Focus bottom | user-custom (`keymaps.lua:13`) | bound: `ctrl+k j` focus below | no: Ctrl+J = Enter same byte |  |
| `<C-k>` | n | Focus top | user-custom (`keymaps.lua:14`) | bound: `ctrl+k k` focus above; also Noni `ctrl+k` prefix | risky: Ctrl+K prefix chord in Noni |  |
| `<C-l>` | n | Focus right | user-custom (`keymaps.lua:15`) | bound: `ctrl+k l` | yes |  |
| `<C-Left>` etc | n | same as hjkl window focus | user-custom (`keymaps.lua:16-19`) | missing (arrows editor motion) | yes |  |
| `<C-h/j/k/l>` | t | leave term + focus window | user-custom (`keymaps.lua:21-24`) | hardcoded: terminal Esc/Ctrl+] leave; Ctrl-A–Z bytes PTY ko | risky |  |
| `<Esc>` | t | `<C-\><C-n>` | user-custom (`keymaps.lua:25`) | hardcoded: terminal Esc → `return_to_normal` (PTY Esc nahi) | yes |  |
| `<A-Up/Down>` | n | `:resize ±2` (file order pehle; dump me overwrite) | user-custom (`keymaps.lua:27-28`) | bound: `ctrl+k _/=` height | no: Alt |  |
| `<A-Left/Right>` | n | `:vertical resize ±2` | user-custom (`keymaps.lua:29-30`) | bound: `ctrl+k [/]` width | no: Alt |  |
| `<C-\>` | — | (nvim file me nahi) | — | bound: `ctrl+backslash` split right | yes (from_raw raw 28) |  |

## Buffers / Tabs

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<leader>bn` | n | `:bnext` | user-custom (`keymaps.lua:33`) | bound: `g t` → `workbench.action.nextEditor`; also `:bn` | yes |  |
| `<leader>bb` | n | `:bprevious` | user-custom (`keymaps.lua:34`) | bound: `g shift+t`; `:bp` | yes |  |
| `<S-h>` | n | BufferLineCyclePrev | plugin:bufferline (`ui.lua:234`) | bound: `g shift+t` | yes |  |
| `<S-l>` | n | BufferLineCycleNext | plugin:bufferline (`ui.lua:235`) | bound: `g t` | yes |  |
| `<leader>bd` | n | `:bdelete` | plugin:bufferline (`ui.lua:236`) | command exists, unbound (`workbench.action.closeActiveEditor` bound `ctrl+w`); `:bd` | yes |  |
| `<leader>bp` | n | BufferLineTogglePin | plugin:bufferline (`ui.lua:237`) | missing | yes |  |
| `<leader>bo` | n | BufferLineCloseOthers | plugin:bufferline (`ui.lua:238`) | missing | yes |  |
| `<leader>be` | n | Neotree buffers float | plugin:neo-tree (`editor.lua:19`) | bound: `space ;` buffer picker | yes |  |
| `<leader>q` | n | `:q` quit window | user-custom (`keymaps.lua:6`) | command exists, unbound (`workbench.action.quit`); `:q` | yes |  |
| `<leader>qq` | n | `:qa` | user-custom (`keymaps.lua:7`) | missing as chord (ex `:qa` nahi sys.txt V me) | yes |  |

## Files / Search

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<leader><Space>` | n | FzfLua files | plugin:fzf-lua (`editor.lua:91`) | bound: `ctrl+p` → `noni.search.files`; `space` is Noni leader | yes |  |
| `<leader>ff` | n | FzfLua files | plugin:fzf-lua (`editor.lua:92`) | bound: `ctrl+p` / `:find` | yes |  |
| `<leader>fg` | n | FzfLua live_grep | plugin:fzf-lua (`editor.lua:93`) | bound: `ctrl+shift+f` / `space shift+f` findInFiles | `ctrl+shift+f`: no: Ctrl+Shift often not delivered; `space shift+f`: yes |  |
| `<leader>fw` | n | FzfLua grep_cword | plugin:fzf-lua (`editor.lua:94`) | missing (project search panel query) | yes |  |
| `<leader>fb` | n | FzfLua buffers | plugin:fzf-lua (`editor.lua:95`) | bound: `space ;` | yes |  |
| `<leader>fr` | n | FzfLua oldfiles | plugin:fzf-lua (`editor.lua:96`) | missing (RecentFiles exist, picker command nahi) | yes |  |
| `<leader>fh` | n | FzfLua help_tags | plugin:fzf-lua (`editor.lua:97`) | missing (`:help` alag) | yes |  |
| `<leader>fc` | n | FzfLua command_history | plugin:fzf-lua (`editor.lua:98`) | missing | yes |  |
| `/` `?` `n` `N` | n | buffer search | nvim-builtin (editor.cpp Noni me hardcoded) | hardcoded: `editor.cpp` | yes |  |

## Explorer

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<leader>e` | n | Neotree filesystem toggle left | plugin:neo-tree (`editor.lua:17`) | bound: `space e` → `workbench.view.explorer` | yes |  |
| `<leader>o` | n | Neotree filesystem focus | plugin:neo-tree (`editor.lua:18`) | conflict: `space o` → `workbench.action.openWorkspace` | yes |  |
| neo-tree `l` | neo-tree window | open | plugin:neo-tree (`editor.lua:66`) | hardcoded: sidebar `l` expand/open (`sidebar.cpp`) | yes |  |
| neo-tree `h` | neo-tree window | close_node | plugin:neo-tree (`editor.lua:67`) | hardcoded: sidebar `h` | yes |  |
| neo-tree `<space>` | neo-tree window | `"none"` (leader steal nahi) | plugin:neo-tree (`editor.lua:65`) | conflict: sidebar Space toggle dir, KeybindingEngine leader steal | yes |  |
| neo-tree `Y` | neo-tree window | copy path to `+` | plugin:neo-tree (`editor.lua:68-75`) | missing | yes |  |
| `<leader>ge` | n | Neotree git_status float | plugin:neo-tree (`editor.lua:20`) | missing | yes |  |

## LSP / Code

Buffer-local `LspAttach` (`coding.lua:46-63`). Global dump me nahi.

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `gd` | n (LSP buf) | `vim.lsp.buf.definition` | user-custom (`coding.lua:51`) | bound: `g d` / `f12` → `editor.action.revealDefinition` | yes |  |
| `gD` | n (LSP buf) | declaration | user-custom (`coding.lua:52`) | bound: `g shift+d` | yes |  |
| `gi` | n (LSP buf) | implementation | user-custom (`coding.lua:53`) | missing | yes |  |
| `K` | n (LSP buf) | hover | user-custom (`coding.lua:54`) | missing (hover LSP method nahi, sys.txt X) | yes |  |
| `<leader>rn` | n (LSP buf) | rename | user-custom (`coding.lua:55`) | bound: `f2` / `space l n` | yes |  |
| `<leader>ca` | n (LSP buf) | code action | user-custom (`coding.lua:56`) | bound: `space l a` | yes |  |
| `[d` `]d` | n (LSP buf) | prev/next diagnostic + float | user-custom (`coding.lua:57-62`) | missing as chords; diagnostics gutter draw hai | yes |  |
| `<leader>ld` | n | FzfLua lsp_definitions | plugin:fzf-lua (`editor.lua:99`) | bound: `g d` | yes |  |
| `<leader>lr` | n | FzfLua lsp_references | plugin:fzf-lua (`editor.lua:100`) | conflict: `space l r` → `lsp.restart` (refs = `g r` / `shift+f12`) | yes |  |
| `<leader>ls` | n | FzfLua lsp_document_symbols | plugin:fzf-lua (`editor.lua:101`) | conflict: `space l s` → `lsp.showStatus` (symbols = `space l o`) | yes |  |
| `<leader>lw` | n | FzfLua workspace symbols | plugin:fzf-lua (`editor.lua:102`) | bound: `space l w` | yes |  |
| `<leader>lx` | n | FzfLua diagnostics_workspace | plugin:fzf-lua (`editor.lua:103`) | missing | yes |  |
| `<leader>cf` | n | conform format | plugin:conform (`coding.lua:119-125`) | missing (format LSP nahi, sys.txt X) | yes |  |
| `<leader>F` | n | conform format | plugin:conform (`coding.lua:127-132`) | missing | yes |  |
| `<leader>xx` | n | Trouble diagnostics toggle | plugin:trouble (`editor.lua:151`) | missing | yes |  |
| `<leader>xX` | n | Trouble buffer diagnostics | plugin:trouble (`editor.lua:152`) | missing | yes |  |
| `<leader>xs` | n | Trouble symbols | plugin:trouble (`editor.lua:153`) | missing | yes |  |
| `<leader>xl` | n | Trouble lsp | plugin:trouble (`editor.lua:154`) | missing | yes |  |
| `<leader>xq` | n | Trouble qflist | plugin:trouble (`editor.lua:155`) | missing | yes |  |
| `<C-space>` | i (blink preset enter) | show / docs toggle | plugin:blink.cmp (`presets.lua:74`; user `keymap = { preset = "enter" }` coding.lua:102) | bound: `ctrl+space` → `editor.action.triggerSuggest` (`when`: `editorFocus`) | risky: Ctrl+Space fragile (NUL / tmux) |  |
| `<CR>` | i (blink) | accept completion | plugin:blink.cmp (`presets.lua:76`) | hardcoded: Completion Enter accept (`ui.cpp`) | yes |  |
| `<C-e>` | i (blink) | cancel | plugin:blink.cmp (`presets.lua:75`) | missing | yes |  |
| `<C-p>` `<C-n>` | i (blink) | select prev/next | plugin:blink.cmp (`presets.lua:82-83`) | hardcoded: completion Up/Down wrap | yes |  |
| `<C-b>` `<C-f>` | i (blink) | scroll docs | plugin:blink.cmp (`presets.lua:86-87`) | missing | yes |  |
| `<C-k>` | i (blink) | signature help toggle | plugin:blink.cmp (`presets.lua:89`) | missing (signatureHelp LSP nahi) | risky: Noni Ctrl+K prefix |  |
| `<Tab>` `<S-Tab>` | i (blink) | snippet forward/back | plugin:blink.cmp (`presets.lua:78-79`) | conflict: Tab insert `\t` / completion accept Tab; Normal Tab = sidebar toggle | Ctrl+I=Tab same byte |  |

Runtime extras (dump, user lua nahi): `grn` rename, `gra` code action, `grr` references, `gri` impl, `grt` type def — source: nvim-builtin (`vim/_core/defaults`). Noni: rename/F2, quickfix `space l a`, refs `g r`, type `g y`; implementation missing.

## Git

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `]c` `[c` | n (gitsigns buf) | next/prev hunk | plugin:gitsigns (`editor.lua:126-139`) | missing | yes |  |
| `<leader>gp` | n (gitsigns buf) | preview hunk | plugin:gitsigns (`editor.lua:140`) | missing | yes |  |
| `<leader>gr` | n (gitsigns buf) | reset hunk | plugin:gitsigns (`editor.lua:141`) | conflict: `space g r` → `git.refresh` | yes |  |
| `<leader>gS` | n (gitsigns buf) | stage hunk | plugin:gitsigns (`editor.lua:142`) | missing (file stage `space g a`) | yes |  |
| `<leader>gs` | n | FzfLua git_status | plugin:fzf-lua (`editor.lua:104`) | bound: `space g s` → `git.showStatus` | yes |  |
| `<leader>gc` | n | FzfLua git_commits | plugin:fzf-lua (`editor.lua:105`) | missing | yes |  |
| `<leader>gb` | n | FzfLua git_branches | plugin:fzf-lua (`editor.lua:106`) | missing | yes |  |

## Terminal

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<leader>tt` | n | ToggleTerm float | plugin:toggleterm (`ui.lua:275`) | bound: `ctrl+t` toggle | yes |  |
| `<leader>th` | n | ToggleTerm horizontal size=15 | plugin:toggleterm (`ui.lua:276`) | missing (ek bottom panel) | yes |  |
| `<leader>tv` | n | ToggleTerm vertical size=60 | plugin:toggleterm (`ui.lua:277`) | missing | yes |  |
| `<C-`>` | n, t | ToggleTerm float | plugin:toggleterm (`ui.lua:278`) | command exists, unbound (`workbench.action.terminal.toggle` = `ctrl+t`) | no: Ctrl+` aksar nahi pahunchta |  |
| toggleterm `open_mapping` | — | `false` | plugin:toggleterm (`ui.lua:281`) | — | — |  |

## Comment / Surround / Text-objects

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `gcc` | n | toggle line comment | plugin:Comment.nvim (`config.lua:91`; dump confirms) | missing | yes |  |
| `gbc` | n | toggle block comment | plugin:Comment.nvim (`config.lua:92`) | missing | yes |  |
| `gc` / `gb` | n, x, o | comment operator | plugin:Comment.nvim | missing | yes |  |
| `gcO` `gco` `gcA` | n | comment insert above/below/eol | plugin:Comment.nvim extra | missing | yes |  |
| `ys` `yss` `yS` `ySS` | n | add surround | plugin:nvim-surround (`plugin/nvim-surround.lua:38-48`) | missing | yes |  |
| `ds` | n | delete surround | plugin:nvim-surround (`:50`) | missing | yes |  |
| `cs` `cS` | n | change surround | plugin:nvim-surround (`:53-57`) | missing | yes |  |
| `S` `gS` | x | visual surround | plugin:nvim-surround (`:61-66`) | missing | yes |  |
| `<C-g>s` `<C-g>S` | i | insert surround | plugin:nvim-surround (`:30-35`) | missing | yes |  |
| `(` `[` `{` `"` `'` `` ` `` | i | mini.pairs open/close | plugin:mini.pairs (dump imap) | missing | yes |  |
| `<BS>` `<CR>` | i | MiniPairs.bs / cr | plugin:mini.pairs (dump) | hardcoded: insert Backspace/Enter | yes |  |

## UI toggles / Session / Misc

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `<leader>ss` | n | persistence.load | plugin:persistence (`ui.lua:292-297`) | bound: `space m r` restoreSession | yes |  |
| `<leader>sl` | n | persistence last | plugin:persistence (`ui.lua:300-304`) | missing | yes |  |
| `<leader>sd` | n | persistence.stop | plugin:persistence (`ui.lua:307-311`) | missing | yes |  |
| `<leader>ms` / `<leader>mr` Noni | — | — | — | bound: `space m s` saveSession; nvim `<leader>mr` = macro | conflict: nvim `space m r` macro vs Noni session restore | yes |  |
| alpha `f/n/g/r/e/c/s/l/q` | alpha dashboard only | find/new/grep/recent/explorer/config/session/Lazy/quit | plugin:alpha-nvim (`ui.lua:80-89`) | N/A (no start screen) | yes |  |

---

# VSCODE

## (a) Custom `keybindings.json`

2 add + 2 remove (`-command`).

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| `ctrl+d` | `textInputFocus && !editorReadonly` | `editor.action.deleteLines` | VSCode user `keybindings.json` | hardcoded: Normal `Ctrl-D` (raw 4) half-page (`editor.cpp`) | conflict: VSCode delete-line vs Noni half-page | yes |  |
| `ctrl+shift+k` | same | `-editor.action.deleteLines` (unbind default) | VSCode user | — | no: Ctrl+Shift |  |
| `ctrl+e` | `editorTextFocus && !editorReadonly` | `editor.action.copyLinesDownAction` | VSCode user | command exists, unbound | yes |  |
| `ctrl+shift+alt+down` | same | `-editor.action.copyLinesDownAction` (unbind default) | VSCode user | — | no: Ctrl+Shift+Alt |  |

## (b) Settings / extensions (keybinding-related)

`settings.json` me **nahi**: `vim.*`, `vim.leader`, `vim.normalModeKeyBindings*`, `vim.insertModeKeyBindings*`, `vim.handleKeys`, `editor.multiCursorModifier`, `terminal.integrated.commandsToSkipShell`.

Keybinding-adjacent settings jo file me hain:
- `editor.formatOnSave`: true
- `editor.suggestOnTriggerCharacters`: true
- `editor.inlineSuggest.enabled`: false
- `files.autoSave`: afterDelay
- `workbench.sideBar.location`: left
- `workbench.statusBar.visible`: false
- `workbench.commandCenter.enabled`: false
- `chat.commandCenter.enabled`: false

**`code --list-extensions`:** VSCodeVim / vscode-neovim **nahi**. Keymap-related jo list me hai: `ms-toolsai.jupyter-keymap`. Baaki themes, language packs, prettier, python, java, remote, live-server, etc. (full list dump me; vim emulation nahi).

## (c) Default VSCode shortcuts (Linux)

source: **default (unverified)** — in files nahi.

| Key | Mode/When | Action | Source | Noni status | Terminal-safe | KEEP? |
|---|---|---|---|---|---|---|
| Ctrl+S | editor | Save | default (unverified) | bound: `workbench.action.files.save` | yes |  |
| Ctrl+K S | editor | Save all | default (unverified) | missing; Noni `ctrl+k s` = split down | conflict: `ctrl+k s` split down | yes |  |
| Ctrl+P | workbench | Quick open | default (unverified) | bound: `noni.search.files` | yes |  |
| Ctrl+N | workbench | New file | default (unverified) | missing (untitled via CLI/session) | yes |  |
| Ctrl+W | workbench | Close editor | default (unverified) | bound: `workbench.action.closeActiveEditor` (`normalMode`) | yes |  |
| Ctrl+Z | edit | Undo | default (unverified) | bound: `u` undo (normal); insert me nahi | yes |  |
| Ctrl+Y / Ctrl+Shift+Z | edit | Redo | default (unverified) | bound: `ctrl+y` redo (`normalMode`); insert Ctrl-R hardcoded redo | Ctrl+Y yes; Ctrl+Shift+Z no |  |
| Ctrl+X/C/V | edit | Cut/copy/paste | default (unverified) | paste: `ctrl+v` normal + registers `p`; cut/copy vim `d`/`y` | yes |  |
| Ctrl+A | edit | Select all | default (unverified) | missing | yes |  |
| Ctrl+Shift+K | edit | Delete line (user unbound this) | default (unverified) | missing as delete-line | no: Ctrl+Shift |  |
| Alt+Up / Alt+Down | edit | Move line | default (unverified) | missing | no: Alt |  |
| Shift+Alt+Up/Down | edit | Copy line | default (unverified); user moved copy-down to Ctrl+E | missing | no: Alt |  |
| Ctrl+] / Ctrl+[ | edit | Indent / outdent | default (unverified) | missing as indent; Ctrl+] leave terminal (raw 29) | Ctrl+] Noni terminal leave |  |
| Ctrl+/ | edit | Toggle line comment | default (unverified) | missing | no: raw 31 from_raw me mapped nahi (user rule / sys.txt T2) |  |
| Shift+Alt+A | edit | Toggle block comment | default (unverified) | missing | no: Alt |  |
| Shift+Alt+F | edit | Format document | default (unverified) | missing | no: Alt |  |
| Ctrl+D | edit | Add selection to next find match | default (unverified); **user remapped to deleteLines** | hardcoded: Ctrl-D half-page | conflict | yes |  |
| Ctrl+Shift+L | edit | Select all occurrences | default (unverified) | missing | no: Ctrl+Shift |  |
| Alt+Click | edit | Add cursor | default (unverified) | missing (no mouse) | — |  |
| Ctrl+F | search | Find | default (unverified) | hardcoded: `/` buffer search | yes |  |
| Ctrl+H | search | Replace | default (unverified) | command exists, unbound (`editor.action.replaceOne/All`) | no: Ctrl+H = Backspace |  |
| F3 / Shift+F3 | search | Find next/prev | default (unverified) | hardcoded: `n`/`N`; commands unbound | Shift+F3: no Shift+F; F3: from_raw KEY_F(3) parse_single has f3 |  |
| Ctrl+Shift+F | search | Find in files | default (unverified) | bound: `ctrl+shift+f` + `space shift+f` | no for ctrl+shift+f; `space shift+f` yes |  |
| Ctrl+G | nav | Go to line | default (unverified) | missing | yes |  |
| Ctrl+Shift+O | nav | Go to symbol in file | default (unverified) | bound: `space l o` | no: Ctrl+Shift |  |
| Ctrl+T | nav | Go to symbol in workspace | default (unverified) | conflict: `ctrl+t` → terminal.toggle; symbols = `space l w` | yes (reaches) / meaning conflict |  |
| F12 | nav | Go to definition | default (unverified) | bound: `f12` | yes |  |
| Alt+F12 | nav | Peek definition | default (unverified) | missing | no: Alt |  |
| Shift+F12 | nav | References | default (unverified) | bound: `shift+f12` + `g r` | no: from_raw F12 pe shift bit nahi (sys.txt T2) |  |
| F2 | nav | Rename | default (unverified) | bound: `f2` | yes |  |
| Ctrl+. | nav | Quick fix | default (unverified) | bound: `space l a` | yes |  |
| Ctrl+Space | nav | Trigger suggest | default (unverified) | bound: `ctrl+space` (`editorFocus`) | risky: Ctrl+Space |  |
| Ctrl+Shift+Space | nav | Parameter hints | default (unverified) | missing | no: Ctrl+Shift |  |
| Alt+Left / Alt+Right | nav | Back / forward | default (unverified) | hardcoded: Ctrl-O / Tab jump list (Tab shadowed) | no: Alt |  |
| F8 / Shift+F8 | nav | Next/prev problem | default (unverified) | missing | Shift+F8 no; F8 search panel regex toggle (hardcoded) |  |
| Ctrl+B | layout | Toggle sidebar | default (unverified) | bound: `ctrl+b` | yes |  |
| Ctrl+` | layout | Toggle terminal | default (unverified) | bound: `ctrl+t` instead | no: Ctrl+` |  |
| Ctrl+\\ | layout | Split editor | default (unverified) | bound: `ctrl+backslash` | yes |  |
| Ctrl+1/2/3 | layout | Focus editor group | default (unverified) | bound: `ctrl+k h/j/k/l` | yes |  |
| Ctrl+PageDown / Ctrl+PageUp | layout | Next/prev tab | default (unverified) | bound: `g t` / `g shift+t` | yes |  |
| Ctrl+= / Ctrl+- | layout | Zoom | default (unverified) | missing | yes |  |
| Ctrl+Shift+` | terminal | New terminal | default (unverified) | command exists, unbound (`terminal.focus`) | no: Ctrl+Shift |  |
| (kill terminal) | terminal | Kill | default (unverified) often palettes | command exists, unbound (`workbench.action.terminal.kill`); `:term kill` | — |  |
| Ctrl+Shift+P | workbench | Command palette | default (unverified) | missing (ex `:` command line) | no: Ctrl+Shift |  |
| Ctrl+, | workbench | Settings | default (unverified) | missing (config.json file) | yes |  |

---

# OVERLAPS

Same kaam, alag keys (preference nahi):

| Action | nvim (is config) | VSCode | Noni (abhi) |
|---|---|---|---|
| Save | `<C-s>` n/i, `<leader>w` | Ctrl+S | `ctrl+s` (`editorFocus`) |
| Quick open files | `<leader><Space>` / `<leader>ff` | Ctrl+P | `ctrl+p` |
| Find in files | `<leader>fg` | Ctrl+Shift+F | `ctrl+shift+f` + `space shift+f` |
| Toggle explorer | `<leader>e` | Ctrl+B (sidebar) | `space e` explorer; `ctrl+b` toggle sidebar |
| Focus explorer | `<leader>o` | — | conflict: `space o` = open workspace |
| Buffers list | `<leader>fb` / `<leader>be` | Ctrl+P / Ctrl+Tab | `space ;` |
| Next/prev buffer | `<S-l>` / `<S-h>`, `<leader>bn`/`bb` | Ctrl+PageDown/Up | `g t` / `g shift+t` |
| Close editor | `<leader>bd` | Ctrl+W | `ctrl+w` (normal) |
| Toggle terminal | `<leader>tt`, `<C-`>` | Ctrl+` | `ctrl+t` |
| Split | (nvim `<C-w>v/s` builtin, user file me nahi) | Ctrl+\\ | `ctrl+backslash` / `ctrl+k s` |
| Definition | `gd`, `<leader>ld` | F12 | `g d` / `f12` |
| Declaration | `gD` | — | `g shift+d` |
| Type definition | nvim-builtin `grt` (dump) | — | `g y` |
| References | `<leader>lr`; dump `grr` | Shift+F12 | `g r` / `shift+f12` |
| Rename | `<leader>rn`; dump `grn` | F2 | `f2` / `space l n` |
| Code action | `<leader>ca`; dump `gra` | Ctrl+. | `space l a` |
| Hover | `K` (LspAttach) | hover on dwell / Ctrl+K Ctrl+I (unverified) | missing |
| Document symbols | `<leader>ls` | Ctrl+Shift+O | `space l o` (nvim `space l s` = status in Noni) |
| Workspace symbols | `<leader>lw` | Ctrl+T | `space l w` (nvim/VSCode Ctrl+T vs Noni terminal) |
| Format | `<leader>cf` / `<leader>F` | Shift+Alt+F | missing |
| Completion | blink `<C-space>` / Enter | Ctrl+Space | `ctrl+space` (`editorFocus`) |
| Line comment | `gcc` | Ctrl+/ | missing |
| Move line | `<A-j>`/`<A-k>` | Alt+Up/Down | missing |
| Delete line | — | user Ctrl+D (was Ctrl+Shift+K) | Ctrl-D = half-page |
| Copy line down | — | user Ctrl+E | unbound |
| Undo | `u` (builtin) | Ctrl+Z | `u` (normal bound) |
| Redo | (nvim `Ctrl-R` builtin) | Ctrl+Y | `ctrl+y` + insert Ctrl-R |
| Session restore | `<leader>ss` | — | `space m r` |
| Git status | `<leader>gs` | SCM view (skipped) | `space g s` |
| Git stage | `<leader>gS` hunk | — | `space g a` file |
| Leave insert | `jk` | — | Esc + insert `jj` |

---

# OPEN DECISIONS

(sirf sawal)

1. Editor Insert mode me start ho ya Normal me?
2. Leader Space rahe?
3. Ctrl shortcuts Insert mode me bhi chalein? (abhi config me zyadatar `editorFocus && normalMode`; `ctrl+s` / `ctrl+space` / `escape` exceptions)

---

## Counts (is file ke tables, unique key+mode rows approx)

- nvim rows (user-custom + plugin keys + comment/surround/blink/gitsigns/neo-tree window): tables upar
- VSCode custom adds: **2** (`ctrl+d`, `ctrl+e`); custom unbinds: **2**
- VSCode default (unverified) table: listed set in (c)
)
