#!/usr/bin/env bash

cd "$(dirname "$0")"
cd ..

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

# macOS editor (arm64 only, not stripped)
scons $SCONS_FLAGS platform=osx target=release_debug arch=arm64
if [ ! -n "$CLEAN_STRING" ]; then
  cp -r misc/dist/osx_tools.app ./bin/${EXE_NAME}.app
  mkdir -p ./bin/${EXE_NAME}.app/Contents/MacOS
  cp ./bin/godot.osx.opt.tools.arm64 ./bin/${EXE_NAME}.app/Contents/MacOS/Godot
  chmod +x ./bin/${EXE_NAME}.app/Contents/MacOS/Godot
  mv ./bin/${EXE_NAME}.app ./bin/exes
fi

if [ -n "$CLEAN_STRING" ]; then
  rm -rf ./bin
fi

