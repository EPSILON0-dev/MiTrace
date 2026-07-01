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

GLTF_FILE=../../tests/material_spheres_test/material_spheres.gltf
OUT_FILE=material_spheres_render.png

find_mitrace_executable
$MITRACE `dirname $0`/$GLTF_FILE -o `dirname $0`/$OUT_FILE -q \
    -w 512 -h 512 -s 128 -e 9 \
    $@