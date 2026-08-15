#!/bin/bash

# Download and install Tremulous game data and assets
#
# This script handles two concerns:
# 1. Build-time: Ensures UI header files (menudef.h, etc.) are available for compilation
# 2. Runtime:    Downloads game data PK3 files needed to actually play the game
#
# The build-time UI headers come from the assets/ git submodule (tremulous-assets repo).
# The runtime PK3 data files are downloaded from GitHub releases.

# Use first command-line argument if provided, else take first directory under build/
dir=${1:-$( find ./build -maxdepth 1 -type d | head -2 | tail -1 )}

echo "Installing data and assets under $dir"
pushd "$dir"

download() {
    local url=$1
    local file=$2

    if [ -r ../$file ]; then
        echo "Using existing $file"
    else
        echo "Downloading $url and saving as $file"
        curl -L --output ../$file $url
    fi
}

# ============================================================================
# Build-time: Copy UI headers from assets submodule to build directory
# ============================================================================
if [ -d ../assets/ui ]; then
    echo "Copying UI header files from assets submodule"
    mkdir -p ../build/assets/ui
    cp ../assets/ui/*.h ../build/assets/ui/ 2>/dev/null || true
else
    echo "WARNING: assets/ui/ directory not found. Build may fail."
    echo "Make sure the tremulous-assets submodule is initialized:"
    echo "  git submodule update --init --recursive"
fi

# ============================================================================
# Runtime: Download tremulous-1.1.0 and grhub assets with base maps
# ============================================================================
DATA_110="data-1.1.0.zip"
download "https://github.com/GrangerHub/tremulous-data/archive/refs/tags/v1.1.0.zip" $DATA_110

# named "grhub" so it comes after "gpp"
DATA_GRHUB="data-grhub-1.3.1.pk3"
download "https://github.com/GrangerHub/tremulous-assets/releases/download/data-1.3.1/tremulous-grhub-1.3.1.pk3" \
    $DATA_GRHUB

if [[ $dir == *darwin* ]]; then
    subdir=./Tremulous.app/Contents/MacOS/gpp/
else
    subdir=./gpp
fi

echo "Extracting 1.1.0 data"
mkdir -p $subdir
unzip -jo -d $subdir ../$DATA_110 "*.pk3"

echo "Adding Grangerhub assets"
cp ../$DATA_GRHUB $subdir/

popd # out of dir

echo "Extracting UI assets for build process"
mkdir -p build/assets
unzip -o -d build/assets build/$DATA_GRHUB "ui/*.h"
