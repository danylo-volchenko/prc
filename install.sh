#!/usr/bin/env bash

# An installer for the 'prc' C/C++ project manager.

# --- Configuration ---
# Use XDG standards for destination paths.
INSTALL_DIR="${HOME}/.local/bin"
TEMPLATE_DEST_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/prc/template"

# --- Colors for Output ---
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# --- Main Installation Logic ---
echo -e "${GREEN}--- Installing 'prc' Project Manager ---${NC}"

# Check for dependencies
echo "Checking for dependencies (git, make, gcc/g++, direnv)..."
for cmd in git make gcc g++ direnv; do
    if ! command -v "$cmd" &> /dev/null; then
        echo -e "${RED}Error: Dependency '$cmd' not found. Please install it first.${NC}"
        exit 1
    fi
done
echo "All dependencies found."

# Create destination directories
echo "Creating installation directories..."
mkdir -p "$INSTALL_DIR"
mkdir -p "$TEMPLATE_DEST_DIR"

# Copy the 'prc' script
echo "Installing 'prc' script to ${INSTALL_DIR}..."
cp ./prc "${INSTALL_DIR}/prc"
chmod +x "${INSTALL_DIR}/prc"

# Copy the project template
echo "Installing project template to ${TEMPLATE_DEST_DIR}..."
# Use rsync for cleaner copying and future updates
rsync -a --delete ./template/ "$TEMPLATE_DEST_DIR/"

# Check if the installation directory is in the user's PATH
if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
    echo -e "\n${YELLOW}--- POST-INSTALL ACTION REQUIRED ---${NC}"
    echo -e "Your PATH does not seem to include ${INSTALL_DIR}."
    echo -e "Please add the following line to your shell configuration file (e.g., ~/.zshrc, ~/.bashrc):"
    echo -e "\n  export PATH=\"\$HOME/.local/bin:\$PATH\"\n"
    echo -e "Then, restart your shell for the change to take effect."
else
    echo -e "\n${GREEN}Installation complete!${NC}"
    echo -e "You can now use the 'prc' command globally."
fi
