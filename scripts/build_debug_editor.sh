#!/usr/bin/env bash

cd "$(dirname "$0")"
cd ..

set -e

JOB_COUNT=$(sysctl -n hw.logicalcpu)
EXE_NAME="Godot+WasmEngine"
export BUILD_NAME="wasm_engine"
export GODOT_VERSION_STATUS="dev"

if [[ " ${@} " =~ " --clean " ]]; then
  CLEAN_STRING="--clean"
else
  CLEAN_STRING=""
fi
SCONS_FLAGS="${CLEAN_STRING} --jobs=${JOB_COUNT}"



mkdir -p ./bin/export_templates
mkdir -p ./bin/exes

# macOS editor (arm64 only, not stripped, debug)
scons $SCONS_FLAGS platform=osx target=debug arch=arm64

if [ -n "$CLEAN_STRING" ]; then
  rm -rf ./bin/exes
  rm -rf ./bin/export_templates
  rm -rf ./bin/godot.*
fi

