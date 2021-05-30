#!/bin/bash
failed=0

BASEMOD=trem13 USE_RESTCLIENT=1 USE_INTERNAL_LUA=1 make -j 2 || failed=1

if [[ $failed -eq 1 ]]; then
    echo "Build failure."
    exit $failed
fi

./misc/download-paks.sh
REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/.."
P=$(uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
PLATFORM=${PLATFORM:-$P}
for dir in ${REPO_ROOT}/build/*; do
  if [[ ! -d $dir ]]; then
      continue;
  fi

  if [[ $PLATFORM != "darwin" ]]; then
      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "trem13_11/vm/*"
      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "trem13/vm/*"
      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "trem13/cgame.*"
      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "trem13/ui.*"
  fi
done
chmod -R ugo+rw build
