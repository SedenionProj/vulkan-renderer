#!/bin/bash

GLSLC=glslc
OUTPUT_DIR="spv"

mkdir -p "$OUTPUT_DIR"
echo "Created folder: $OUTPUT_DIR"

echo "Starting shader compilation..."
echo "-------------------------------"

for file in *.vert *.frag; do
    [[ -e "$file" ]] || continue  # Skip if no files match

    name="${file%.*}"
    ext="${file##*.}"

    if [[ "$ext" == "vert" ]]; then
        stage="vert"
        suffix="Vert.spv"
    elif [[ "$ext" == "frag" ]]; then
        stage="frag"
        suffix="Frag.spv"
    else
        continue
    fi

    echo "Compiling $file to $OUTPUT_DIR/${name}${suffix}"
    "$GLSLC" -fshader-stage=$stage -I . "$file" -o "$OUTPUT_DIR/${name}${suffix}"
done

echo "-------------------------------"
echo "Compilation complete"
