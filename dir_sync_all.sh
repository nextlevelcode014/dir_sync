#!/bin/bash

BIN="$HOME/bin/dir_sync"

$BIN "$HOME/dev/sysconfig/nvim/" "$HOME/.config/nvim/"  &
$BIN "$HOME/.var/app/net.ankiweb.Anki/data/Anki2/" "$HOME/Backups/Anki2/" &

wait

