#!/bin/sh
SCENE_FILENAME=golf_in_the_end.gltf

if test -z "$MITRACE_RELATIVE_PATH"; then
    MITRACE_RELATIVE_PATH=../../../build
fi
MITRACE_EXECUTABLE="$(pwd)/$(dirname "$0")/$MITRACE_RELATIVE_PATH/MiTrace"
if $MITRACE_EXECUTABLE --version | grep MiTrace >/dev/null; then true; else
    echo "Invalid MiTrace executable: $MITRACE_EXECUTABLE"
fi

SCENE_PATH=$(dirname $0)/$SCENE_FILENAME
ENV_PATH=$(dirname $0)/tex/env.png
RENDER_PATH=$(dirname $0)/$(basename $SCENE_FILENAME .gltf)_render.png

$MITRACE_EXECUTABLE $SCENE_PATH -o $RENDER_PATH \
    -w 360 -h 240 -s 32 -e 8.5 -b 3 -v \
      -hdri $ENV_PATH \
      --camera 10 \
      --hdri-primary-intensity 140 \
      --hdri-secondary-intensity 1000 \
      --hdri-rotation 90 \
      --emission-base-intensity 2000 \
      --disable-firefly-elimination \
      --bvh-max-depth 48 \
      --image-block-size 16 \
      $@
