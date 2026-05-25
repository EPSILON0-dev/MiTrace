#!/bin/sh

cd "$(dirname "$0")"
RENDER_SCRIPTS=`ls */render.sh`
while read -r RENDER_SCRIPT; do
    printf "\033[1m\033[34m[render_all.sh] \033[0m"
    echo "Rendering $(basename $(dirname "$RENDER_SCRIPT"))..."
    sh "$RENDER_SCRIPT" $@
done <<< "$RENDER_SCRIPTS"
