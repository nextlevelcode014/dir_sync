#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>

#define MAX_WATCHES 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

char source_dir[PATH_MAX];
char target_dir[PATH_MAX];

typedef struct {
    int wd;
    char path[PATH_MAX];
} Watch;

Watch watch_list[MAX_WATCHES];
int watch_count = 0;

/* ========== UTILITÁRIOS ========== */

// Copia arquivo
// [ A B C D E F G H ]
// char buf[4];
// fread(buf, 1, 4, file);  // lê A B C D
// fread(buf, 1, 4, file);  // lê E F G H
// fread(buf, 1, 4, file);  // retorna 0 → fim do arquivo
void copy_file(const char *src_path, const char *dst_path) {
    FILE *src = fopen(src_path, "rb");
    FILE *dst = fopen(dst_path, "wb");
    if (!src || !dst) {
        perror("copy_file fopen");
        if (src) fclose(src);
        if (dst) fclose(dst);
        return;
    }

    char buf[4096];
    size_t size;
    while ((size = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, size, dst);
    }

    fclose(src);
    fclose(dst);
}

// Log com timestamp
void log_event(const char *message, const char *src, const char *dst) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "[%Y-%m-%d %H:%M:%S]", t);
    printf("%s %s: %s → %s\n", ts, message, src, dst);
}

// Cria diretórios recursivamente
// *p aponta para os indereços de temp e os modifica.
void mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// Sincroniza conteúdo de origem para destino (recursivo)
void sync_directories(const char *src, const char *dst) {
    DIR *dir = opendir(src);
    if (!dir) return;

    mkdir_p(dst);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char src_path[PATH_MAX], dst_path[PATH_MAX];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        struct stat st;
        if (stat(src_path, &st) < 0) continue;

        if (S_ISDIR(st.st_mode)) {
            sync_directories(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            copy_file(src_path, dst_path);
        }
    }

    closedir(dir);
}

// Remove do destino
void remove_from_target(const char *relative_path) {
    char dst[PATH_MAX];
    snprintf(dst, sizeof(dst), "%s/%s", target_dir, relative_path);
    remove(dst);
}

/* ========== WATCH RECURSIVO ========== */

// Adiciona um diretório para monitorar
void add_watch(int fd, const char *path) {
    int wd = inotify_add_watch(fd, path, IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0) {
        perror("inotify_add_watch");
        return;
    }

    if (watch_count < MAX_WATCHES) {
        watch_list[watch_count].wd = wd;
        snprintf(watch_list[watch_count].path, sizeof(watch_list[watch_count].path), "%s", path);
        watch_count++;
    }
}

// Adiciona recursivamente todos os diretórios
void add_watch_recursive(int fd, const char *path) {
    add_watch(fd, path);

    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            add_watch_recursive(fd, full_path);
        }
    }

    closedir(dir);
}

// Retorna caminho de um watch descriptor
const char* get_watch_path(int wd) {
    for (int i = 0; i < watch_count; i++) {
        if (watch_list[i].wd == wd) return watch_list[i].path;
    }
    return NULL;
}

/* ========== MAIN ========== */

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <source_dir> <target_dir>\n", argv[0]);
        return 1;
    }

    realpath(argv[1], source_dir);
    realpath(argv[2], target_dir);

    sync_directories(source_dir, target_dir);

    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        perror("inotify_init1");
        return 1;
    }

    add_watch_recursive(fd, source_dir);
    printf("🛡️  Monitorando alterações em: %s\n", source_dir);

    char buffer[EVENT_BUF_LEN];

    while (1) {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length <= 0) {
            usleep(100000); // 100ms
            continue;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];

            const char *watched_path = get_watch_path(event->wd);
            if (!watched_path || !event->len) {
                i += EVENT_SIZE + event->len;
                continue;
            }

            char full_src[PATH_MAX], full_dst[PATH_MAX];
            snprintf(full_src, sizeof(full_src), "%s/%s", watched_path, event->name);
            snprintf(full_dst, sizeof(full_dst), "%s/%s", target_dir, full_src + strlen(source_dir) + 1);

            struct stat st;
            int is_dir = (stat(full_src, &st) == 0 && S_ISDIR(st.st_mode));

            if (event->mask & (IN_CREATE | IN_MOVED_TO)) {
                if (is_dir) {
                    add_watch_recursive(fd, full_src);
                    sync_directories(full_src, full_dst);
                    log_event("📁 Nova pasta", full_src, full_dst);
                } else {
                    copy_file(full_src, full_dst);
                    log_event("📝 Arquivo criado", full_src, full_dst);
                }
            } else if (event->mask & IN_MODIFY && !is_dir) {
                copy_file(full_src, full_dst);
                log_event("✏️ Modificado", full_src, full_dst);
            } else if (event->mask & (IN_DELETE | IN_MOVED_FROM)) {
                remove(full_dst);
                log_event("🗑️ Deletado", full_src, full_dst);
            }

            i += EVENT_SIZE + event->len;
        }
    }

    close(fd);
    return 0;
}

