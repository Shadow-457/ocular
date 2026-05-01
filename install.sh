#!/bin/bash
if [ "$EUID" -ne 0 ]
  then echo "Please run as root to install system-wide (e.g. sudo ./install.sh)"
  exit
fi

echo "Installing Ocular..."

# Rebuild to ensure it's up to date
make

# Copy executable
install -Dm755 ocular /usr/local/bin/ocular

# Copy desktop entry and icon for system recognition
install -Dm644 ocular.desktop /usr/share/applications/ocular.desktop
install -Dm644 ocular.svg /usr/share/icons/hicolor/scalable/apps/ocular.svg

echo "Updating icon cache and desktop database..."
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
update-desktop-database /usr/share/applications 2>/dev/null || true

echo "Installation complete! You can now launch Ocular from your application menu or by running 'ocular' in the terminal."
