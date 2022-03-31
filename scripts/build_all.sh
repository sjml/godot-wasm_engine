#!/usr/bin/env bash

set -e

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
SCONS_FLAGS="${CLEAN_STRING} --jobs=${JOB_COUNT} production=yes"



mkdir -p ./bin/export_templates
mkdir -p ./bin/exes


# macOS editor (universal)
scons $SCONS_FLAGS platform=osx target=release_debug arch=arm64 
scons $SCONS_FLAGS platform=osx target=release_debug arch=x86_64
./bin/godot.osx.opt.tools.arm64 --version | tail -1 > ./bin/export_templates/version.txt
if [ ! -n "$CLEAN_STRING" ]; then
  strip ./bin/godot.osx.opt.tools.arm64
  strip ./bin/godot.osx.opt.tools.x86_64 
  lipo -create ./bin/godot.osx.opt.tools.arm64 ./bin/godot.osx.opt.tools.x86_64 -output ./bin/godot.osx.opt.tools.universal
  cp -r misc/dist/osx_tools.app ./bin/${EXE_NAME}.app
  mkdir -p ./bin/${EXE_NAME}.app/Contents/MacOS
  cp ./bin/godot.osx.opt.tools.universal ./bin/${EXE_NAME}.app/Contents/MacOS/Godot
  chmod +x ./bin/${EXE_NAME}.app/Contents/MacOS/Godot
  rm -rf ./bin/exes/${EXE_NAME}.app
  mv ./bin/${EXE_NAME}.app ./bin/exes
fi

# macOS export templates (universal)
scons $SCONS_FLAGS platform=osx tools=no target=release       arch=arm64 
scons $SCONS_FLAGS platform=osx tools=no target=release_debug arch=arm64 
scons $SCONS_FLAGS platform=osx tools=no target=release       arch=x86_64
scons $SCONS_FLAGS platform=osx tools=no target=release_debug arch=x86_64
if [ ! -n "$CLEAN_STRING" ]; then
  strip ./bin/godot.osx.opt.arm64
  strip ./bin/godot.osx.opt.x86_64
  strip ./bin/godot.osx.opt.debug.arm64
  strip ./bin/godot.osx.opt.debug.x86_64
  lipo -create ./bin/godot.osx.opt.arm64       ./bin/godot.osx.opt.x86_64       -output ./bin/godot.osx.opt.universal
  lipo -create ./bin/godot.osx.opt.debug.arm64 ./bin/godot.osx.opt.debug.x86_64 -output ./bin/godot.osx.opt.debug.universal
  cp -r misc/dist/osx_template.app ./bin/osx_template.app
  mkdir -p ./bin/osx_template.app/Contents/MacOS
  cp ./bin/godot.osx.opt.universal       ./bin/osx_template.app/Contents/MacOS/godot_osx_release.64
  cp ./bin/godot.osx.opt.debug.universal ./bin/osx_template.app/Contents/MacOS/godot_osx_debug.64
  chmod +x ./bin/osx_template.app/Contents/MacOS/godot_osx*
  cd bin
  zip -q -9 -r osx.zip osx_template.app
  cd ..
  mv ./bin/osx.zip ./bin/export_templates
fi


# windows editor
scons $SCONS_FLAGS platform=windows target=release_debug arch=x86_64 bits=64 use_mingw=true
if [ ! -n "$CLEAN_STRING" ]; then
  x86_64-w64-mingw32-strip ./bin/godot.windows.opt.tools.x86_64.exe
  cp ./bin/godot.windows.opt.tools.x86_64.exe ./bin/exes/${EXE_NAME}.exe
	cp ./modules/wasm_engine/wasmtime/lib/x86_64-windows/wasmtime.dll ./bin/exes/
fi

# windows export templates
scons $SCONS_FLAGS platform=windows tools=no target=release       arch=x86_64 bits=64 use_mingw=true
scons $SCONS_FLAGS platform=windows tools=no target=release_debug arch=x86_64 bits=64 use_mingw=true
if [ ! -n "$CLEAN_STRING" ]; then
  x86_64-w64-mingw32-strip ./bin/godot.windows.opt.x86_64.exe
  x86_64-w64-mingw32-strip ./bin/godot.windows.opt.debug.x86_64.exe
  cp ./bin/godot.windows.opt.x86_64.exe       ./bin/export_templates/windows_64_release.exe
  cp ./bin/godot.windows.opt.debug.x86_64.exe ./bin/export_templates/windows_64_debug.exe
	cp ./modules/wasm_engine/wasmtime/lib/x86_64-windows/wasmtime.dll ./bin/export_templates/
fi

if [ -n "$CLEAN_STRING" ]; then
  rm -rf ./bin
fi

