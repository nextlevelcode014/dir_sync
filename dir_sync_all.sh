#!/bin/bash

DIR="$(dirname "$(readlink -f "$0")")"
BIN="$DIR/dir_sync"

# If you have multiple instances, some of the directory names listed may conflict, so instead of exporting the variable globally, you can do this.

BLACKLIST="$DIR/blacklist.txt"

BLACKLIST_PATH="$BLACKLIST" $BIN "$HOME/dev/sysconfig/nvim/" "$HOME/.config/nvim/"  &

wait

