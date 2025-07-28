#ifndef WATCHER_H
#define WATCHER_H

#include <sys/inotify.h> 
#include <limits.h>

#define MAX_WATCHES 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

/** 
 * @brief Represents a directory being watched by inotify.
 * **/
typedef struct {
    int wd; /**< Watch descriptor returned by inotify_add_watch */
    char path[PATH_MAX]; /**< Path of the watched directory */
} Watch;

/**
 * @brief List of all watched directories.
 *
 **/
extern Watch watch_list[MAX_WATCHES];


/**
 * @brief Total number of directories currently being wached.
 **/
extern int watch_count;

/**
 * @brief Adds a directory to watch list.
 *
 * Associates a watch descriptor with a directory path and stores it in watch list.
 *
 * @param fd File descriptor returned by inotify_init.
 * @param path Path of the directory to watch.
 **/
void add_watch(int fd, const char *path);

/**
 * @brief Recursively adds a directory and all its subdirectories to the watch list.
 *
 * Opens the given directory, iterates through its entries, and adds subdirectories recursively.
 *
 ** * @param fd File descriptor returned by inotify_init.
 * @param path Root directory path to start watching from.
 **/
void add_watch_recursive(int fd, const char *path);

/**
 * @brief Returns the path associated with a given path descriptor.
 *
 * Searches watch_list for the specified watch descriptor.
 *
 * @param wd Watch descriptor.
 * @return Corresponding directory path, or NULL if not found.
 **/
const char* get_watch_path(int wd);

#endif
