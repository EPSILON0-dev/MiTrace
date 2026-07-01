#!/bin/sh

function find_mitrace_executable() {
    if test -z "$MITRACE_RELATIVE_PATH"; then
        MITRACE_RELATIVE_PATH=../../build
    fi
    MITRACE="$(pwd)/$(dirname "$0")/$MITRACE_RELATIVE_PATH/MiTrace"
    if $MITRACE --version | grep MiTrace >/dev/null; then true; else
        echo "Invalid MiTrace executable: $MITRACE"
    fi

    export MITRACE
}

function render_test() {
    TEST_DIR=$1
    shift

    SCENE_FILENAME=$(ls $TEST_DIR/*.gltf | grep -v "raw" | head -n 1)
    PARAMS_FILENAME=$(ls $TEST_DIR/*.txt | head -n 1)
    SCENE_PATH=$(dirname $0)/$SCENE_FILENAME
    PARAMS_PATH=$(dirname $0)/$PARAMS_FILENAME
    OUTPUT_PATH=$(dirname $0)/outputs/$(basename $SCENE_FILENAME .gltf)_render.png

    params=$(cat $PARAMS_PATH)

    $MITRACE $SCENE_PATH -o $OUTPUT_PATH --quiet $params $@
    echo
}

TEST_DIRECTORIES=`ls -d *test/`

find_mitrace_executable

for TEST_DIR in $TEST_DIRECTORIES; do
    printf "\033[1m\033[34m[render_all.sh] \033[0m"
    echo "Rendering $TEST_DIR..."
    render_test $TEST_DIR $@
done