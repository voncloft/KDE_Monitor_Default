# KDE Primary Monitor Detector (Qt6/C++)

Small Qt6 GUI app that shows the current primary monitor on Linux/KDE.

It does **not** use `xorg.conf`. Instead it uses:
- `QGuiApplication::primaryScreen()` (KDE/Qt display state)
- best-effort KDE KScreen DBus query (`org.kde.KScreen`)

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
./build/kde-primary-monitor
```

The app window shows:
- session type (`x11`/`wayland`)
- Qt/KDE primary monitor
- best-effort KScreen DBus primary output
- all detected screens in a table
- live updates when monitors are added/removed or primary changes
