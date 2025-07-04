#ifndef UTILS_H
#define UTILS_H

#define MAX_LINE 256

/**
 * @brief Adds two integers.
 * 
 * @param a First integer.
 * @param b Second integer.
 * @return int Sum of a and b.
 */
void copy_file(const char *src_path, const char *dst_path);
void log_event(const char *message, const char *src, const char *dst);
void mkdir_p(const char *path);
void sync_directories(const char *src, const char *dst);
void remove_from_target(const char *relative_path);
int is_temporary_file(const char *filename);
int blacklist(const char *black_dir);

#endif
