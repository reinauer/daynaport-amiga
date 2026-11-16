#!/bin/sh
# Builds scsidayna.device with m68k-amigaos-gcc (default).
# To build with VBCC instead: COMPILER=vbcc ./build.sh
if [ "$COMPILER" = "vbcc" ]; then
	export VBCC=${VBCC:-../../vbcc}
	export PATH=$PATH:$VBCC/bin
	exec make compiler_vcc=1 "$@"
fi

exec make "$@"
