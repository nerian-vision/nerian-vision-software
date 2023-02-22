#!/bin/bash

# Deploy NVCom as an AppImage using linuxdeploy (place it in PATH)
# https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

LINUXDEPLOY_BIN="linuxdeploy-x86_64.AppImage"
which "$LINUXDEPLOY_BIN" >/dev/null 2>&1 || { echo "Not in path: $LINUXDEPLOY_BIN, cannot package AppImage."; exit 1; }

[ -e bin/nvcom ] || { echo "NVCom not built yet, cannot package AppImage."; exit 1; }

# Just place the temporary AppDir inside build/ so that it gets cleaned as well
APPDIR=`pwd`/build/AppDir
mkdir -p "$APPDIR"/usr/bin
mkdir -p "$APPDIR"/usr/share/icons/hicolor/256x256
mkdir -p "$APPDIR"/usr/share/applications

# Manually copy. Also possible with DESTDIR=$APPDIR make install, but in that case
#  PREFIX=/usr (i.e. CMAKE_INSTALL_PREFIX=/usr) is needed, unlike our default /usr/local
cp bin/nvcom "$APPDIR"/usr/bin/
cp icons/program-icon-256.png "$APPDIR"/usr/share/icons/hicolor/256x256/nvcom.png
cat >"$APPDIR"/usr/share/applications/nvcom.desktop <<EOF
[Desktop Entry]
Type=Application
Name=NVCom
Comment=NVCom, by Nerian Vision GmbH
Exec=nvcom
Icon=nvcom
Categories=Video;AudioVideo;
EOF

# Enter bin directory -> resulting AppImage file will then also be placed here
cd bin

# Deploy: fetch dependencies and package
"$LINUXDEPLOY_BIN" --appdir="$APPDIR" --desktop-file="$APPDIR"/usr/share/applications/nvcom.desktop --output appimage \
    && { echo 'Success.'; exit 0; } || { echo 'ERROR!'; exit 1; }

