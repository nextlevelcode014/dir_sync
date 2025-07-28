#!/bin/bash

DIR="$(dirname "$(readlink -f "$0")")"
BIN="$DIR/dir_sync"

$BIN "$HOME/dev/sysconfig/nvim/" "$HOME/.config/nvim/"  &

wait

