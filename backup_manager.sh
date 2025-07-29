#!/bin/bash

# Backup Manager Script
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
BACKUP_SCRIPT="$SCRIPT_DIR/backup_sync.sh"
LOCKFILE_DIR="/tmp"

usage() {
    cat << EOF
Backup Manager - Manage multiple backup instances

Usage: $0 [COMMAND] [OPTIONS]

COMMANDS:
    start       Start a new backup instance
    stop        Stop a specific backup instance
    status      Show status of all backup instances
    list        List all running backup instances
    killall     Stop all backup instances

START OPTIONS:
    -s, --source DIR        Source directory
    -e, --external          Use external media
    -l, --local DIR         Use local directory as destination  
    -d, --destination DIR   Custom destination directory

STOP OPTIONS:
    -d, --destination DIR   Stop backup for specific destination
    -p, --pid PID          Stop backup by process ID

EXAMPLES:
    $0 start --external                          # Start external backup
    $0 start --local /backup/docs                # Start local backup
    $0 start -s /home/user/pics -l /backup/pics  # Custom source to local
    $0 stop -d /run/media/user/BackupDrive/Backups  # Stop specific backup
    $0 status                                    # Show all running backups
    $0 killall                                   # Stop all backups
EOF
}

# Get lock files and extract information
get_running_backups() {
    for lockfile in "$LOCKFILE_DIR"/dir_sync_*.lock; do
        if [ -f "$lockfile" ]; then
            PID=$(cat "$lockfile" 2>/dev/null)
            if [ ! -z "$PID" ] && kill -0 "$PID" 2>/dev/null; then
                # Extract destination from lockfile name
                DEST_ENCODED=$(basename "$lockfile" .lock | sed 's/^dir_sync_//')
                DESTINATION=$(echo "$DEST_ENCODED" | sed 's|_|/|g')
                echo "$PID:$DESTINATION:$lockfile"
            else
                # Remove orphaned lock file
                rm -f "$lockfile"
            fi
        fi
    done
}

start_backup() {
    if [ ! -f "$BACKUP_SCRIPT" ]; then
        echo "Error: Backup script not found at $BACKUP_SCRIPT"
        exit 1
    fi
    
    # Make backup script executable
    chmod +x "$BACKUP_SCRIPT"
    
    echo "Starting backup with arguments: $@"
    "$BACKUP_SCRIPT" "$@" &
    BACKUP_PID=$!
    
    # Give it a moment to start and create lock file
    sleep 2
    
    if kill -0 $BACKUP_PID 2>/dev/null; then
        echo "Backup started successfully (PID: $BACKUP_PID)"
    else
        echo "Failed to start backup"
        exit 1
    fi
}

stop_backup() {
    local TARGET_DEST=""
    local TARGET_PID=""
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--destination)
                TARGET_DEST="$2"
                shift 2
                ;;
            -p|--pid)
                TARGET_PID="$2"
                shift 2
                ;;
            *)
                echo "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    if [ -z "$TARGET_DEST" ] && [ -z "$TARGET_PID" ]; then
        echo "Error: Must specify either --destination or --pid"
        exit 1
    fi
    
    local FOUND=false
    
    while IFS=':' read -r PID DESTINATION LOCKFILE; do
        if [ ! -z "$TARGET_DEST" ] && [ "$DESTINATION" = "$TARGET_DEST" ]; then
            echo "Stopping backup to $DESTINATION (PID: $PID)..."
            kill -TERM "$PID" 2>/dev/null
            FOUND=true
            break
        elif [ ! -z "$TARGET_PID" ] && [ "$PID" = "$TARGET_PID" ]; then
            echo "Stopping backup process $PID (Destination: $DESTINATION)..."
            kill -TERM "$PID" 2>/dev/null
            FOUND=true
            break
        fi
    done < <(get_running_backups)
    
    if [ "$FOUND" = false ]; then
        echo "No matching backup process found"
        exit 1
    fi
}

show_status() {
    echo "Running Backup Instances:"
    echo "========================="
    
    local COUNT=0
    while IFS=':' read -r PID DESTINATION LOCKFILE; do
        COUNT=$((COUNT + 1))
        echo "[$COUNT] PID: $PID"
        echo "    Destination: $DESTINATION"
        echo "    Started: $(stat -c %y "$LOCKFILE" 2>/dev/null | cut -d. -f1)"
        
        # Check if it's external media
        if echo "$DESTINATION" | grep -q "^/run/media\|^/media\|^/mnt"; then
            MEDIA_PATH=$(dirname "$DESTINATION")
            if mountpoint -q "$MEDIA_PATH" 2>/dev/null; then
                echo "    Status: External media mounted"
            else
                echo "    Status: External media NOT mounted (process may be stuck)"
            fi
        else
            echo "    Status: Local backup"
        fi
        echo ""
    done < <(get_running_backups)
    
    if [ $COUNT -eq 0 ]; then
        echo "No backup instances are currently running."
    else
        echo "Total running instances: $COUNT"
    fi
}

list_backups() {
    echo "PID:DESTINATION"
    get_running_backups | cut -d: -f1,2
}

kill_all_backups() {
    echo "Stopping all backup instances..."
    
    local COUNT=0
    while IFS=':' read -r PID DESTINATION LOCKFILE; do
        echo "Stopping backup to $DESTINATION (PID: $PID)..."
        kill -TERM "$PID" 2>/dev/null
        COUNT=$((COUNT + 1))
    done < <(get_running_backups)
    
    if [ $COUNT -eq 0 ]; then
        echo "No backup instances were running."
    else
        echo "Sent termination signal to $COUNT backup instances."
        echo "Waiting for processes to clean up..."
        sleep 3
        
        # Check if any are still running
        REMAINING=$(get_running_backups | wc -l)
        if [ $REMAINING -gt 0 ]; then
            echo "Warning: $REMAINING processes are still running. They may need more time to clean up."
        else
            echo "All backup processes have been stopped successfully."
        fi
    fi
}

# Main script logic
if [ $# -eq 0 ]; then
    usage
    exit 1
fi

COMMAND="$1"
shift

case "$COMMAND" in
    start)
        start_backup "$@"
        ;;
    stop)
        stop_backup "$@"
        ;;
    status)
        show_status
        ;;
    list)
        list_backups
        ;;
    killall)
        kill_all_backups
        ;;
    *)
        echo "Unknown command: $COMMAND"
        usage
        exit 1
        ;;
esac
