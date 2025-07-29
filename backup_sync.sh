#!/bin/bash

# Script configuration
DIR="$(dirname "$(readlink -f "$0")")"
BIN="$DIR/dir_sync"
BLACKLIST="$DIR/blacklist.txt"

# Default paths
DEFAULT_SOURCE="$HOME/dev/dir_sync/source"
DEFAULT_EXTERNAL_MEDIA="/run/media/$USER/5276beb0-233d-407d-8adc-7f186276bfab"
LOCKFILE_DIR="/tmp"

# Usage function
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

OPTIONS:
    -s, --source DIR        Source directory (default: $DEFAULT_SOURCE)
    -d, --destination DIR   Destination directory
    -e, --external          Use external media (default: $DEFAULT_EXTERNAL_MEDIA)
    -l, --local DIR         Use local directory as destination
    -h, --help              Show this help message

EXAMPLES:
    $0 --external                           # Backup to external media
    $0 --local /backup/mydata               # Backup to local directory
    $0 -s /home/user/docs -l /backup/docs   # Custom source to local backup
    $0 -s /home/user/docs -d /custom/path   # Custom source to custom destination

Multiple instances can run simultaneously with different destinations.
EOF
}

# Parse command line arguments
SOURCE="$DEFAULT_SOURCE"
DESTINATION=""
USE_EXTERNAL=false
MONITOR_EXTERNAL=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--source)
            SOURCE="$2"
            shift 2
            ;;
        -d|--destination)
            DESTINATION="$2"
            shift 2
            ;;
        -e|--external)
            USE_EXTERNAL=true
            MONITOR_EXTERNAL=true
            DESTINATION="$DEFAULT_EXTERNAL_MEDIA/Backups"
            shift
            ;;
        -l|--local)
            DESTINATION="$2"
            USE_EXTERNAL=false
            MONITOR_EXTERNAL=false
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Validate arguments
if [ -z "$DESTINATION" ]; then
    echo "Error: No destination specified. Use --external, --local, or --destination"
    usage
    exit 1
fi

if [ ! -d "$SOURCE" ]; then
    echo "Error: Source directory does not exist: $SOURCE"
    exit 1
fi

# Generate unique lock file name based on destination
LOCKFILE_NAME=$(echo "$DESTINATION" | sed 's|/|_|g' | sed 's|^_||')
LOCKFILE="$LOCKFILE_DIR/dir_sync_${LOCKFILE_NAME}.lock"

# Cleanup function
cleanup() {
    echo "Executing cleanup..."
    
    # Remove lock file
    rm -f "$LOCKFILE"
    
    # Kill sync process if still running
    if [ ! -z "$SYNC_PID" ] && kill -0 $SYNC_PID 2>/dev/null; then
        echo "Interrupting sync process (PID: $SYNC_PID)..."
        kill -TERM $SYNC_PID 2>/dev/null
        
        # Wait for graceful termination
        for i in {1..5}; do
            if ! kill -0 $SYNC_PID 2>/dev/null; then
                break
            fi
            sleep 1
        done
        
        # Force termination if necessary
        if kill -0 $SYNC_PID 2>/dev/null; then
            echo "Forcing process termination..."
            kill -KILL $SYNC_PID 2>/dev/null
        fi
        
        echo "Sync process interrupted."
    fi
    
    # Kill monitoring process
    if [ ! -z "$MONITOR_PID" ] && kill -0 $MONITOR_PID 2>/dev/null; then
        kill $MONITOR_PID 2>/dev/null
    fi
    
    exit 0
}

# Check if another process is already running for this destination
if [ -f "$LOCKFILE" ]; then
    OLD_PID=$(cat "$LOCKFILE")
    if kill -0 "$OLD_PID" 2>/dev/null; then
        echo "Another backup process is already running for this destination (PID: $OLD_PID)"
        echo "Destination: $DESTINATION"
        exit 1
    else
        echo "Removing orphaned lock file..."
        rm -f "$LOCKFILE"
    fi
fi

# Create lock file
echo $$ > "$LOCKFILE"

# Setup signal handlers for cleanup
trap cleanup SIGTERM SIGINT EXIT

# Check if external media is mounted (only if using external media)
if [ "$USE_EXTERNAL" = true ]; then
    MEDIA_PATH=$(dirname "$DESTINATION")
    if ! mountpoint -q "$MEDIA_PATH"; then
        echo "External media is not mounted at $MEDIA_PATH"
        exit 1
    fi
    echo "External media detected and mounted at $MEDIA_PATH"
fi

# Check if destination directory is accessible
if [ ! -w "$DESTINATION" ]; then
    if ! mkdir -p "$DESTINATION" 2>/dev/null; then
        echo "Cannot access or create destination directory: $DESTINATION"
        exit 1
    fi
fi

echo "Starting synchronization..."
echo "Source: $SOURCE"
echo "Destination: $DESTINATION"
echo "External media monitoring: $MONITOR_EXTERNAL"

# Start sync process
BLACKLIST_PATH="$BLACKLIST" $BIN "$SOURCE" "$DESTINATION" &
SYNC_PID=$!

echo "Sync process started (PID: $SYNC_PID)"

# Handle monitoring based on backup type
if [ "$MONITOR_EXTERNAL" = true ]; then
    # Media monitoring function for external media
    monitor_external_media() {
        echo "Monitoring external media..."
        MEDIA_PATH=$(dirname "$DESTINATION")
        
        while kill -0 $SYNC_PID 2>/dev/null; do
            # Check if still mounted
            if ! mountpoint -q "$MEDIA_PATH"; then
                echo "External media was removed!"
                return 1
            fi
            
            # Check if destination directory is still accessible
            if [ ! -d "$DESTINATION" ]; then
                echo "Destination directory is no longer accessible!"
                return 1
            fi
            
            # Write test to verify media is still working
            if ! touch "$MEDIA_PATH/.test_write" 2>/dev/null; then
                echo "External media is no longer writable!"
                return 1
            fi
            rm -f "$MEDIA_PATH/.test_write" 2>/dev/null
            
            sleep 2
        done
        return 0
    }
    
    # Start monitoring in background
    monitor_external_media &
    MONITOR_PID=$!
    
    # Wait for either sync process or monitor to finish
    wait -n  # Wait for any background process to finish
    
    # Check which process finished
    if kill -0 $SYNC_PID 2>/dev/null; then
        # If sync is still running, then monitor detected a problem
        echo "Problem detected with external media. Interrupting sync..."
        kill -TERM $SYNC_PID 2>/dev/null
        wait $SYNC_PID
        echo "Synchronization interrupted due to media removal."
        exit 1
    else
        # Sync finished normally
        echo "Synchronization completed successfully."
        if [ ! -z "$MONITOR_PID" ] && kill -0 $MONITOR_PID 2>/dev/null; then
            kill $MONITOR_PID 2>/dev/null
        fi
        exit 0
    fi
else
    # For local backups, just wait for sync to complete
    echo "Waiting for synchronization to complete..."
    wait $SYNC_PID
    SYNC_EXIT_CODE=$?
    
    if [ $SYNC_EXIT_CODE -eq 0 ]; then
        echo "Synchronization completed successfully."
        exit 0
    else
        echo "Synchronization failed with exit code: $SYNC_EXIT_CODE"
        exit $SYNC_EXIT_CODE
    fi
fi
