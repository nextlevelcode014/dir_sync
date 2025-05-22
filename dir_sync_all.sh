#!/bin/bash

BIN="$HOME/bin/dir_sync"

$BIN "$HOME/dev/sysconfig/i3/" "$HOME/.config/i3/" &
$BIN "$HOME/dev/sysconfig/nvim/" "$HOME/.config/nvim/"  &
$BIN "$HOME/dev/sysconfig/fish/config.fish" "$HOME/.config/fish/config.fish" &
$BIN "$HOME/dev/sysconfig/starship/starship.toml" "$HOME/.config/starship.toml" &
$BIN "$HOME/dev/sysconfig/xorg.conf.d/" "/etc/X11/xorg.conf.d/"  &
$BIN "$HOME/dev/sysconfig/optimus-manager/" "/etc/optimus-manager/" &
$BIN "$HOME/dev/sysconfig/picom/" "$HOME/.config/picom/" &
$BIN "$HOME/dev/sysconfig/polybar/" "$HOME/.config/polybar/" &
$BIN "$HOME/dev/sysconfig/rofi/" "$HOME/.config/rofi/" &
$BIN "$HOME/dev/sysconfig/alacritty/" "$HOME/.config/alacritty/" &

wait

