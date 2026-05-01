# Ocular

A simple, fast, and minimal image viewer written in C and GTK+ 3 for Linux. Ocular focuses on providing a lightweight, distraction-free environment for viewing images.

## Features

- **Minimalist Interface**: No clutter, just your images with a dark background.
- **Slideshow**: Automatic slideshow playback for directories.
- **Keyboard Navigation**: Use standard keys (Arrows, Space, `j/k`, `Backspace`) to navigate images.
- **Fullscreen Mode**: Toggle with `F` or `F11`.
- **System Integration**: Recognized by standard Linux application launchers and file managers.

## Requirements

- `gcc`
- `make`
- `gtk+-3.0` development files (e.g., `libgtk-3-dev` on Debian/Ubuntu, `gtk3` on Arch Linux)

## Build and Install

To install system-wide so it appears in your application menu and can be launched from anywhere:

```bash
sudo ./install.sh
```

## Uninstall

If you wish to remove it from your system:

```bash
sudo rm -f /usr/local/bin/ocular
sudo rm -f /usr/share/applications/ocular.desktop
sudo rm -f /usr/share/icons/hicolor/scalable/apps/ocular.svg
sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor
sudo update-desktop-database /usr/share/applications
```

## Usage

```bash
ocular                       # Opens the current directory
ocular /path/to/image.jpg    # Opens a specific image and the surrounding directory
ocular /path/to/folder       # Opens a specific directory
```

## Keyboard Shortcuts

- `Right`, `Space`, `j`: Next image
- `Left`, `Backspace`, `k`: Previous image
- `F`, `F11`: Toggle fullscreen
- `Q`, `Esc`: Quit
