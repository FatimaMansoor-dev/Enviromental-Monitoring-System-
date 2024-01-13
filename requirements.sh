#!/bin/bash

# Install required packages using the package manager for your system

# For Ubuntu/Debian-based systems
sudo apt-get update
sudo apt-get install -y gcc make curl libcurl4-gnutls-dev libgtk-3-dev libcairo2-dev

# For Red Hat/Fedora-based systems
# sudo dnf install -y gcc make curl gtk3-devel cairo-devel libcurl-devel

# Install cJSON
# (Assuming cJSON is included in your project and you don't need a system-wide installation)

# Optionally, install other dependencies based on your project's needs

# You might also need to install other libraries based on your project's specific requirements

echo "Dependencies installed successfully."
