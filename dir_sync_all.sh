#!/bin/bash

BIN="$HOME/bin/dir_sync"

$BIN "$HOME/.config/i3/" "$HOME/dev/sysconfig/i3/" &
$BIN "$HOME/.config/nvim/" "$HOME/dev/sysconfig/nvim/" &
$BIN "$HOME/.config/fish/config.fish" "$HOME/dev/sysconfig/fish/config.fish" &
$BIN "$HOME/.config/starship.toml" "$HOME/dev/sysconfig/starship/starship.toml" &
$BIN "/etc/X11/xorg.conf.d/" "$HOME/dev/sysconfig/xorg.conf.d/" &
$BIN "/etc/optimus-manager/" "$HOME/dev/sysconfig/optimus-manager/" &

wait

