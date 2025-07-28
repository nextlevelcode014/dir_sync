/**
 * @file main.c
 * @brief Main entry point for directory synchronization utility
 * @author Marcelo Augusto
 * @date 2025-01-26
 * @version 1.0
 *
 * This file contains the main program logic for dir_sync, a utility that
 * synchronizes directories recursively and monitors changes in real-time
 * using Linux inotify system. The program performs an initial full
 * synchronization, then continuously monitors the source directory for
 * changes and replicates them to the target directory.
 *
 * Key features:
 * - Recursive directory synchronization
 * - Real-time monitoring using inotify
 * - Filtering of temporary files and blacklisted items
 * - Colored logging with emoji indicators
 * - Support for multiple concurrent instances
 *
 * Usage: ./dir_sync <source_dir> <target_dir>
 *
 * @see README.md for detailed usage examples and setup instructions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include "config.h"
#include "utils.h"
#include "watcher.h"
#include <libgen.h>

/**
 * @brief Main program entry point
 *
 * Initializes the directory synchronization system by:
 * 1. Parsing and validating command line arguments
 * 2. Resolving source and target paths to absolute paths
 * 3. Performing initial full synchronization
 * 4. Setting up inotify monitoring system
 * 5. Processing file system events in continuous loop
 *
 * The program runs indefinitely until interrupted (Ctrl+C) or an
 * unrecoverable error occurs.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 *             - argv[1]: Source directory path
 *             - argv[2]: Target directory path
 *
 * @return 0 on normal termination, 1 on error
 *
 * @note Both source and target paths are resolved to absolute paths
 * @note Source directory must exist and be readable
 * @note Target directory will be created if it doesn't exist
 *
 * Error conditions:
 * - Insufficient command line arguments
 * - Source directory doesn't exist or isn't readable
 * - Cannot initialize inotify system
 * - Cannot add initial watches to source directory
 * - I/O error reading inotify events
 */
int main(int argc, char **argv)
{
    // Validate command line arguments
    if (argc < 3)
    {
        fprintf(stderr, "Use: %s <source_dir> <target_dir>\n", argv[0]);
        fprintf(stderr, "Examplo: %s ~/.config/nvim/ ~/dev/sysconfig/nvim/\n", argv[0]);
        return 1;
    }

    // Resolve paths to absolute paths to avoid issues with relative paths
    // and ensure consistent path handling throughout the program
    if (realpath(argv[1], source_dir) == NULL)
    {
        perror("realpath source_dir");
        fprintf(stderr, "Error: Source directory don't found: %s\n", argv[1]);
        return 1;
    }

    if (realpath(argv[2], target_dir) == NULL)
    {
        // Target directory might not exist yet, try to create parent path
        char parent_dir[PATH_MAX];
        strncpy(parent_dir, argv[2], sizeof(parent_dir) - 1);
        parent_dir[sizeof(parent_dir) - 1] = '\0';

        // Extract parent directory
        char *last_slash = strrchr(parent_dir, '/');
        if (last_slash != NULL)
        {
            *last_slash = '\0';
            mkdir_p(parent_dir);
        }

        // Try realpath again, or use the provided path if it still fails
        if (realpath(argv[2], target_dir) == NULL)
        {
            strncpy(target_dir, argv[2], sizeof(target_dir) - 1);
            target_dir[sizeof(target_dir) - 1] = '\0';
        }
    }

    printf("🚀 Starting synchronization:\n");
    printf("   📂 Source: %s\n", source_dir);
    printf("   📁 Target: %s\n", target_dir);

    // Perform initial full synchronization
    printf("🔄 Performing initial synchornization...\n");
    sync_directories(source_dir, target_dir);
    printf("✅ Initial synchronization complete!\n");

    // Initialize inotify system for real-time monitoring
    int fd = inotify_init();
    if (fd < 0)
    {
        perror("inotify_init");
        fprintf(stderr, "Error: The monitoring system could not be initialized.\n");
        return 1;
    }

    // Add recursive watches to source directory and all subdirectories
    printf("🔍 Configuring recursive monitoring...\n");
    add_watch_recursive(fd, source_dir);
    printf("🛡️  Monitoring changes in: %s\n", source_dir);
    printf("    (Press Ctrl+C to stop)\n\n");

    // Event processing buffer - sized to handle multiple events efficiently
    char buffer[EVENT_BUF_LEN];

    /**
     * Main event processing loop
     *
     * This loop continuously reads inotify events and processes them.
     * Events are read in batches for efficiency, and each event is
     * parsed and handled according to its type.
     */
    while (1)
    {
        // Block until events are available
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length <= 0)
        {
            if (length < 0)
            {
                perror("read inotify events");
            }
            break;
        }

        // Process all events in the buffer
        int i = 0;
        while (i < length)
        {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];

            // Skip events without filename or for filtered files
            // This includes temporary files and blacklisted directories
            if (!event->len || is_temporary_file(event->name) || blacklist(event->name))
            {
                i += EVENT_SIZE + event->len;
                continue;
            }

            // Convert inotify watch descriptor back to filesystem path
            const char *watched_path = get_watch_path(event->wd);
            if (!watched_path || !event->len)
            {
                i += EVENT_SIZE + event->len;
                continue;
            }

            // Build full source and destination paths
            char full_src[8192], full_dst[8192];
            snprintf(full_src, sizeof(full_src), "%s/%s", watched_path, event->name);

            // Calculate relative path from source_dir and build target path
            // This handles nested directories correctly
            snprintf(full_dst, sizeof(full_dst), "%s/%s", target_dir, full_src + strlen(source_dir) + 1);

            // Check if the path refers to a directory
            struct stat st;
            int is_dir = (stat(full_src, &st) == 0 && S_ISDIR(st.st_mode));

            // Handle different types of file system events
            if (event->mask & (IN_CREATE | IN_MOVED_TO))
            {
                // File or directory was created or moved into watched directory
                if (is_dir)
                {
                    // For new directories, add recursive watches and sync contents
                    add_watch_recursive(fd, full_src);
                    sync_directories(full_src, full_dst);
                    log_event("📁 New folder", full_src, full_dst);
                }
                else
                {
                    // For new files, simply copy them
                    mkdir_p(dirname(strdup(full_dst))); // Ensure parent directory exists
                    copy_file(full_src, full_dst);
                    log_event("📝 File created", full_src, full_dst);
                }
            }
            else if (event->mask & IN_MODIFY && !is_dir)
            {
                // File was modified (only files, not directories)
                copy_file(full_src, full_dst);
                log_event("✏️  Modified", full_src, full_dst);
            }
            else if (event->mask & (IN_DELETE | IN_MOVED_FROM))
            {
                // File or directory was deleted or moved away
                remove(full_dst);
                log_event("🗑️  Deleted", full_src, full_dst);
            }

            // Move to next event in buffer
            i += EVENT_SIZE + event->len;
        }
    }

    // Cleanup - close inotify file descriptor
    printf("\n🛑 Ending monitoring...\n");
    close(fd);
    return 0;
}
