#!/bin/bash

APP_NAME=Fuku
XWIN_DIR=~/xwin

cd "$(dirname -- "${BASH_SOURCE[0]}")"

mkdir -p build
cd build

CROSS_COMPILE_OPTIONS=(-fuse-ld=lld /vctoolsdir $XWIN_DIR/crt /winsdkdir $XWIN_DIR/sdk)
WARN_OPTIONS=(
	-Wall
	-Wextra
	-Wshadow
	-Wno-missing-prototypes
	-Wno-missing-variable-declarations
	-Wno-declaration-after-statement
	-Wno-unused-parameter
	-Wno-extra-semi-stmt
	-Wno-cast-qual
	-Wno-unsafe-buffer-usage
)

InvalidArguments() {
	echo "Invalid Arguments. Usage: build.sh [debug | release] [all | platform | game]"
	exit 1
}

COMPILE_OPTIONS=(${CROSS_COMPILE_OPTIONS[@]} ${WARN_OPTIONS[@]} /DAPP_NAME=\"$APP_NAME\" /I../vendor)

if [[ $1 == "debug" ]]; then
	COMPILE_OPTIONS=(${COMPILE_OPTIONS[@]} -g -Od )
elif [[ $1 == "release" ]]; then
	COMPILE_OPTIONS=(${COMPILE_OPTIONS[@]} -O2 )
else
	InvalidArguments
fi

BUILD_PLATFORM=0
BUILD_GAME=0

if [[ $2 == "all" ]]; then
	BUILD_PLATFORM=1
	BUILD_GAME=1
elif [[ $2 == "platform" ]]; then
	BUILD_PLATFORM=1
	BUILD_GAME=0
elif [[ $2 == "game" ]]; then
	BUILD_PLATFORM=0
	BUILD_GAME=1
else
	InvalidArguments
fi

if [[ $BUILD_PLATFORM != 0 ]]; then
	clang-cl ${COMPILE_OPTIONS[@]} ../src/platform_win32.c /link user32.lib gdi32.lib opengl32.lib /incremental:no /out:$APP_NAME.exe
fi
