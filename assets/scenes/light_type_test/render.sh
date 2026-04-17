#!/bin/sh
SCENE_FILENAME=light_type_test.gltf

if test -z "$MITRACE_RELATIVE_PATH"; then
    MITRACE_RELATIVE_PATH=../../../build
fi
MITRACE_EXECUTABLE="$(pwd)/$(dirname "$0")/$MITRACE_RELATIVE_PATH/MiTrace"
if $MITRACE_EXECUTABLE --version | grep MiTrace >/dev/null; then true; else
    echo "Invalid MiTrace executable: $MITRACE_EXECUTABLE"
fi

SCENE_PATH=$(dirname $0)/$SCENE_FILENAME
RENDER_PATH=$(dirname $0)/$(basename $SCENE_FILENAME .gltf)_render.png

$MITRACE_EXECUTABLE $SCENE_PATH -o $RENDER_PATH -w 1280 -h 720 -s 128 --image-block-size 16 -e 7 $@
