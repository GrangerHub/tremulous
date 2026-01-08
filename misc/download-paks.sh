#!/bin/bash

#pushd ./build

# Use first command-line argument if provided, else take first directory under build/
dir=${1:-$( find ./build -maxdepth 1 -type d | head -2 | tail -1 )}

echo "Installing data and assets under $dir"
pushd $dir 

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


# Download tremulous-1.1.0 and gpp assets with base maps
DATA_110="data-1.1.0.zip"
download "https://github.com/GrangerHub/tremulous-data/archive/refs/tags/v1.1.0.zip" $DATA_110

# named "grhub" so it comes after "gpp"
DATA_GRHUB="data-grhub-1.3.0.pk3"
download "https://github.com/GrangerHub/tremulous-assets/releases/download/data-1.3.0/tremulous-grhub-1.3.0.pk3" \
	 $DATA_GRHUB

if [[ $dir == "./build/release-darwin-x86_64" ]]; then
    subdir=./Tremulous.app/Contents/MacOS/gpp/ 
else
    subdir=./gpp
fi

echo "Extracting 1.1.0 data"
unzip -jo -d $subdir ../$DATA_110 "*.pk3"

echo "Adding Grangerhub assets"
cp ../$DATA_GRHUB $subdir/

popd # out of dir

echo "Extracting UI assets for build process"
mkdir -p assets
unzip -o -d assets build/$DATA_GRHUB "ui/*.h"

echo "Done downloading data and assets"
