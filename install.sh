#!/bin/bash

# Sxzip Unix Installation Script (macOS / Linux)

echo "========================================="
echo " Sxzip Installation (macOS / Linux) "
echo "========================================="

# 1. Ensure CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "[!] Error: 'cmake' could not be found. Please install CMake first."
    exit 1
fi

# 2. Build the project
echo "[1/3] Building Sxzip..."
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo "[!] Build failed."
    exit 1
fi

# 3. Install the binary and manual page
echo ""
echo "[2/3] Installing Sxzip to system directories..."
echo "(You may be prompted for your password to copy files to /usr/local/bin and /usr/local/share/man)"
sudo make install

if [ $? -ne 0 ]; then
    echo "[!] Installation failed."
    exit 1
fi

# 4. Success Message
echo ""
echo "[3/3] ✅ Installation Complete!"
echo "-----------------------------------------"
echo "Sxzip is now installed globally."
echo "Try running: sxzip -h"
echo "To read the manual, run: man sxzip"
echo "========================================="
