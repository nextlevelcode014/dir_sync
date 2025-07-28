#include <sys/inotify.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include "watcher.h"
#include "utils.h"
Watch watch_list[MAX_WATCHES];
int watch_count = 0;

void add_watch(int fd, const char *path)
{
    int wd = inotify_add_watch(fd, path, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0)
    {
        perror("inotify_add_watch");
        return;
    }

    if (watch_count < MAX_WATCHES)
    {
        watch_list[watch_count].wd = wd;
        snprintf(watch_list[watch_count].path, sizeof(watch_list[watch_count].path), "%s", path);
        watch_count++;
    }
}

void add_watch_recursive(int fd, const char *path)
{
    add_watch(fd, path);

    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || blacklist(entry->d_name) || is_temporary_file(entry->d_name))
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode))
        {
            add_watch_recursive(fd, full_path);
        }
    }

    closedir(dir);
}

const char *get_watch_path(int wd)
{
    for (int i = 0; i < watch_count; i++)
    {
        if (watch_list[i].wd == wd)
            return watch_list[i].path;
    }
    return NULL;
}
