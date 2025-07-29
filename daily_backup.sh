#!/bin/bash

echo "Starting daily backup routine..."

# Check if external drive is connected
if mountpoint -q /run/media/$USER/5276beb0-233d-407d-8adc-7f186276bfab; then
    echo "Starting external backup..."
    ./backup_manager.sh start --external &
fi

# Start local backups
echo "Starting local backups..."
./backup_manager.sh start -s "$HOME/dev/dir_sync/source" -l "$HOME/dev/dir_sync/target" &

echo "All backup jobs started. Check status with: ./backup_manager.sh status"
