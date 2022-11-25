#!/bin/bash

set -euo pipefail

# determine loda executable based on OS/architecture
if [[ "$OSTYPE" == linux* ]]; then
  arch=$(lscpu | grep "Architecture" | awk '{print $2}')
  if [[ "$arch" == "x86_64" ]] || [[ "$arch" == "i686" ]]; then
    LODA_EXEC="loda-linux-x86"
  elif [[ "$arch" == "aarch64" ]]; then
    LODA_EXEC="loda-linux-arm64"
  fi
elif [[ "$OSTYPE" == darwin* ]]; then
  LODA_EXEC="loda-macos"
fi

if [ -z ${LODA_EXEC+x} ]; then
  echo "Unsupported OS/architecture"
  exit 1
fi

# check if git is installed
if ! [ -x "$(command -v git)" ]; then
  echo "git is not installed. Please install it and retry."
  echo "For more information on git, see https://git-scm.com/"
  exit 1
fi

if [ -z ${LODA_HOME+x} ]; then
  LODA_HOME=$HOME/loda
fi

# download loda and start setup
mkdir -p $LODA_HOME/bin
cd $LODA_HOME/bin
if ! [ -x "./loda" ]; then
  curl -fsSLo loda https://github.com/loda-lang/loda-cpp/releases/latest/download/$LODA_EXEC
  chmod u+x loda
fi
./loda setup
