#!/bin/bash

REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/.."

cd $REPO_ROOT

default_assets_path="${REPO_ROOT}/default-assets"

# prepare the default-assets repo
git submodule init
git submodule update

for dir in ${REPO_ROOT}/build/*; do
    if [[ ! -d $dir ]]; then
        continue;
    fi

    # Copy files

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        pushd $dir/Tremulous.app/Contents/MacOS/base/ 
    else
        mkdir -pv $dir/base
        pushd $dir/base
    fi

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        for i in $default_assets_path/base/*.pk3; do
            cp $i "${PWD}/"
        done
    else
        cp -av $default_assets_path/base/*.pk3 ./
    fi

    popd

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        pushd $dir/Tremulous.app/Contents/MacOS/gpp/ 
    else
        mkdir -pv $dir/gpp
        pushd $dir/gpp
    fi

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        for i in $default_assets_path/gpp/*.pk3; do
            cp $i "${PWD}/"
        done
    else
        cp -av $default_assets_path/gpp/*.pk3 ./
    fi

    popd

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        pushd $dir/Tremulous.app/Contents/MacOS/trem13/ 
    else
        mkdir -pv $dir/trem13
        pushd $dir/trem13
    fi

    mv *.pk3 $dir/
    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        for i in $default_assets_path/trem13/*.pk3; do
            cp $i "${PWD}/"
        done
    else
        cp -a $default_assets_path/trem13/*.pk3 ./
    fi

    popd

    # Repackage and remove the non-default pk3 files from trem13/

    pushd $dir

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        zip -d ${REPO_ROOT}/build/$(basename $dir).zip "Tremulous.app/Contents/MacOS/trem13/*.pk3"
        zip -r ${REPO_ROOT}/build/$(basename $dir).zip Tremulous.app
    else
        zip -r ${REPO_ROOT}/build/$(basename $dir).zip base/*.pk3
        zip -r ${REPO_ROOT}/build/$(basename $dir).zip gpp/*.pk3
        zip -d ${REPO_ROOT}/build/$(basename $dir).zip "trem13/*.pk3"
        zip -r ${REPO_ROOT}/build/$(basename $dir).zip trem13/*.pk3
    fi

    popd

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        pushd $dir/Tremulous.app/Contents/MacOS/trem13/ 
    else
        pushd $dir/trem13
    fi

    if [[ $dir == "${REPO_ROOT}/build/release-macos-64" ]]; then
        for i in $dir/*.pk3; do
            cp $i "${PWD}/"
        done
    else
        cp -an $dir/*.pk3 ./
    fi
    rm -rf $dir/*.pk3

    popd
done
