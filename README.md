# mded

mded is a lightweight Markdown editor for Linux, macOS, and Windows.
It focuses on fast typing, syntax highlighting, and clean reading while editing.

## What You Can Do

- Edit Markdown with multi-tab workflow.
- Highlight Markdown, code fences, tables, links/images, and LaTeX.
- Use dark/white theme toggle from the toolbar.
- Auto-continue list items on Enter (bullets, numbers, checkboxes).
- Save files with standard keyboard shortcuts.

## Quick Start

### Linux

Build and run from source:

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
./build/mded
```

Open files directly:

```bash
./build/mded notes.md todo.md
```

### macOS

Install dependencies (Homebrew example):

```bash
brew install cmake qt
```

Build:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build -j$(sysctl -n hw.ncpu)
```

Run:

```bash
open build/mded.app
```

### Windows

Install Qt 6 and CMake, then build from a Developer PowerShell:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build --config Release
./build/Release/mded.exe
```

## Shortcuts

- `Ctrl+O`: Import/Open file
- `Ctrl+S`: Save
- `Ctrl+Shift+S`: Save As
- `Ctrl+T`: New tab
- `Ctrl+W`: Close tab
- `Ctrl+Tab`: Next tab
- `Ctrl+Shift+Tab`: Previous tab
- `Ctrl+F`: Search
- `Ctrl++` / `Ctrl+-` / `Ctrl+0`: Font zoom in/out/reset
- `Ctrl+L`: Toggle line numbers
- `Ctrl+Shift+W`: Toggle word wrap
- `F11`: Focus mode
- `F12`: Fullscreen

## Themes

- Use the toolbar button `Theme: ...` to switch between dark and white.
- Theme files are stored in [themes](themes).

## Config File

User settings are saved at:

- Linux (typical): `~/.config/mded/config.toml`
- macOS (typical): `~/Library/Preferences/mded/config.toml`
- Windows (typical): `%LOCALAPPDATA%\\mded\\config.toml`

## Acknowledgements

This project was developed with AI assistance. The following projects were referenced for design decisions and architecture during development:

- [Ghostwriter](https://github.com/KDE/ghostwriter) (GPL-3.0) - editor structure, line number area, focus mode
- [QOwnNotes](https://github.com/pbek/QOwnNotes) (GPL-2.0) - syntax highlighter design
- [Zettlr](https://github.com/Zettlr/Zettlr) (GPL-3.0) - CJK text handling, display math state
- [lite-xl](https://github.com/lite-xl/lite-xl) (MIT) - UI minimalism philosophy
- [render-markdown.nvim](https://github.com/MeanderingProgrammer/render-markdown.nvim) (MIT) - token color semantics
- [Marktext](https://github.com/marktext/marktext) (MIT) - theme structure

See [NOTICE](./NOTICE) for full copyright notices.

## License

GPL-3.0
