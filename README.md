# mded — Minimal Markdown Editor

A minimal Markdown + LaTeX syntax highlighting editor for Linux, built with Qt6/C++17.

## Features

- **Pure text editing** — syntax highlighted via colors and font sizes, no rendering
- **Multi-tab** — open multiple `.md` files with independent undo stacks
- **Mouse-friendly** — click, select, scroll, all work as expected
- **Low latency** — incremental highlighting via Qt's `QSyntaxHighlighter`
- **LaTeX support** — inline `$...$`, display `$$...$$`, and `\begin{env}` highlighting
- **CJK support** — proper Chinese/Japanese/Korean boundary handling for bold/italic
- **Catppuccin themes** — dark and light themes built-in, customizable via TOML

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6
make -j$(nproc)
```

## Run

```bash
./mded                    # Open with empty tab
./mded file1.md file2.md  # Open multiple files
```

## Configuration

Settings are stored in `~/.config/mded/config.toml`. See `spec.md` for all options.

## License

GPL-3.0
