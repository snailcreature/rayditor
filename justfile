set lazy

build-cmd := "gcc src/main.c -o bin/main -Wall -Wextra -lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
dev-args := "-fsanitize=address -g -Og"

build:
    #!/usr/bin/env bash
    set -euo pipefail

    if [ ! -d "./build/" ]; then
        mkdir ./build
    fi

    if [ ! -d "./bin/" ]; then
        mkdir ./bin
    fi

    {{ build-cmd }} 

build-gui:
    #!/usr/bin/env bash
    set -euo pipefail

    if [ -e "./src/raygui.h" ]; then
        exit 0;
    fi

    cp ~/Documents/dev/raygui/src/raygui.h ./src/raygui.h

run: build
    ./bin/main

dev:
    #!/usr/bin/env bash
    set -euo pipefail

    echo "building..."
    {{ build-cmd }} {{ dev-args }}
    
    echo "running..."
    ./bin/main
