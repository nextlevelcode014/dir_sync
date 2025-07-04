#ifndef WATCHER_H
#define WATCHER_H

#include <sys/inotify.h> 
#include <limits.h>

#define MAX_WATCHES 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

typedef struct {
    int wd;
    char path[PATH_MAX];
} Watch;

extern Watch watch_list[MAX_WATCHES];
extern int watch_count;

void add_watch(int fd, const char *path);
void add_watch_recursive(int fd, const char *path);
const char* get_watch_path(int wd);

#endif
