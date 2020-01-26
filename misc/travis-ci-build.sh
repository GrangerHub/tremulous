#!/bin/bash
P=$(uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
PLATFORM=${PLATFORM:-$P}
failed=0

if [[ $PLATFORM == "darwin" ]]; then
    rm -rf build
    BASEMOD=trem13 USE_FREETYPE=0 USE_RESTCLIENT=1 USE_INTERNAL_LUA=1 make -j 2 release || failed=1    
    ./misc/download-paks.sh
    REPO_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )/.."
    for dir in ${REPO_ROOT}/build/*; do
      if [[ ! -d $dir ]]; then
          continue;
      fi

      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "Tremulous.app/Contents/MacOS/trem13/cgame.*"
      zip -d ${REPO_ROOT}/build/$(basename $dir).zip "Tremulous.app/Contents/MacOS/trem13/ui.*"
    done
fi

if [[ $failed -eq 1 ]]; then
	echo "Build failure."
else
	echo "Build successful."
fi

exit $failed
