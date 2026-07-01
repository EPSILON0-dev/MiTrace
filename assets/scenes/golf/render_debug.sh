#!/bin/bash

MODES='
primary-bvh-tests 
total-bvh-tests 
albedo 
metallic-roughness 
geometric-normal 
shading-normal 
direct-only 
indirect-only 
emission
color-per-mesh 
color-per-triangle 
color-per-bounding-volume 
bounces 
depth 
first-hit-fresnel 
first-hit-brdf 
first-hit-pdf 
reflected-direction 
pixel-standard-deviation 
firefly-elimination'

GLTF_FILE=golf_in_the_end.gltf

function find_mitrace_executable() {
    if test -z "$MITRACE_RELATIVE_PATH"; then
        MITRACE_RELATIVE_PATH=../../../build
    fi
    MITRACE="$(pwd)/$(dirname "$0")/$MITRACE_RELATIVE_PATH/MiTrace"
    if $MITRACE --version | grep MiTrace >/dev/null; then true; else
        echo "Invalid MiTrace executable: $MITRACE"
    fi

    export MITRACE
}

function render_mode() {
    mode=$1
    shift
    ENV_PATH=`dirname $0`/tex/env.png
    OUT_FILE=`dirname $0`/render-$mode.png
    $MITRACE `dirname $0`/$GLTF_FILE -o `dirname $0`/$OUT_FILE -q \
        -w 360 -h 240 -s 256 -e 8.5 \
        -hdri $ENV_PATH \
        --camera 10 \
        --hdri-primary-intensity 140 \
        --hdri-secondary-intensity 1000 \
        --hdri-rotation 90 \
        --emission-base-intensity 2000 \
        --bvh-max-depth 48 \
        --image-block-size 8 \
        --debug-mode debug-$mode \
        $@
}

find_mitrace_executable
for MODE in $MODES; do
    echo "Rendering debug mode: $MODE"
    render_mode $MODE $@
done
python `dirname $0`/../scripts/stitch_renders.py