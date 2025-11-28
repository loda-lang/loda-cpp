#!/bin/bash
#
# Install external tools for LODA testing (Linux only)
# This script installs:
# - PARI/GP (from source)
# - Lean (from official installation script)
#

set -e

# Check if running on Linux
if [[ "$(uname -s)" != "Linux" ]]; then
    echo "Error: This script only supports Linux systems"
    exit 1
fi

# Create temporary directory for downloads
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo "Installing external tools for LODA tests..."
echo "Temporary directory: $TEMP_DIR"
echo ""

# Install PARI/GP
echo "=== Installing PARI/GP ==="
echo ""

# PARI/GP version to install
PARI_VERSION="2.17.3"
PARI_TAR="pari-${PARI_VERSION}.tar.gz"
PARI_URL="https://pari.math.u-bordeaux.fr/pub/pari/unix/${PARI_TAR}"

echo "Downloading PARI/GP ${PARI_VERSION}..."
cd "$TEMP_DIR"
wget -q --show-progress "$PARI_URL" || curl -L -O "$PARI_URL"

echo "Extracting PARI/GP..."
tar -xzf "$PARI_TAR"
cd "pari-${PARI_VERSION}"

echo "Configuring PARI/GP..."
./Configure --prefix=/usr/local

echo "Building PARI/GP (this may take several minutes)..."
make -j$(nproc)

echo "Installing PARI/GP..."
sudo make install

echo "PARI/GP installation complete!"
echo ""

# Verify PARI/GP installation
if command -v gp &> /dev/null; then
    echo "PARI/GP version:"
    gp --version-short
    echo ""
else
    echo "Warning: gp command not found in PATH"
    echo ""
fi

# Install Lean
echo "=== Installing Lean ==="
echo ""

# Check if elan (Lean version manager) is already installed
if command -v elan &> /dev/null; then
    echo "Elan (Lean version manager) is already installed"
    echo "Updating Lean..."
    elan self update
    elan update
else
    echo "Installing Lean via elan (Lean version manager)..."
    cd "$TEMP_DIR"
    
    # Download and run the elan installation script
    curl -sSf https://raw.githubusercontent.com/leanprover/elan/master/elan-init.sh -o elan-init.sh
    chmod +x elan-init.sh
    
    # Run the installer with default options (non-interactive)
    ./elan-init.sh -y --default-toolchain stable
    
    # Source the environment
    if [ -f "$HOME/.elan/env" ]; then
        source "$HOME/.elan/env"
    fi
fi

echo "Lean installation complete!"
echo ""

# Verify Lean installation
if command -v lean &> /dev/null; then
    echo "Lean version:"
    lean --version
    echo ""
else
    echo "Warning: lean command not found in PATH"
    echo "You may need to add ~/.elan/bin to your PATH:"
    echo "  export PATH=\"\$HOME/.elan/bin:\$PATH\""
    echo ""
fi

echo "=== Installation Summary ==="
echo ""
echo "All external tools have been installed successfully!"
echo ""
echo "PARI/GP: $(command -v gp &> /dev/null && echo 'Installed' || echo 'Not found in PATH')"
echo "Lean:    $(command -v lean &> /dev/null && echo 'Installed' || echo 'Not found in PATH')"
echo ""
echo "To enable external tool tests, set the environment variable:"
echo "  export LODA_TEST_WITH_EXTERNAL_TOOLS=1"
echo ""
echo "Then run tests with:"
echo "  ./loda test"
echo ""
