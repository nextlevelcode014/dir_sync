#ifndef UTILS_H
#define UTILS_H

#define MAX_LINE 256

/**
 * @brief Copies a file from source path to the destination path
 *
 * Opens the source file for reading and writes its contents to the destination file.
 *
 * @param src_path Path to the source file.
 * @param dst_path Path to the destination file.
 * */
void copy_file(const char *src_path, const char *dst_path);

/**
 * @brief Logs an event indicating a change between source and destination files.
 *
 * Tipically used to track modifications during synchronization. 
 *
 * @param message Description of the event.
 * @param src Path to the source file.
 * @param dst Path to the destination file.
 * */
void log_event(const char *message, const char *src, const char *dst);

/*
 *  @brief Create nested directories (like `mkdir -p`).
 *
 *  If intermediate directories do not exist, they are created automatically.
 *
 *  @param path Path of the directory to create.
 * */
void mkdir_p(const char *path);

/**
 * @brief Recursively synchronizes two directories
 *
 * All entries from the source directory are copied to the destination directory.
 * If the destination does not exist, it is created.
 * Temporary files, hidden files, and blacklisted entries (as defined in `BLACKLIST_PATH`) are skipped)
 *
 * @param src Source directory (entries will be copied from here).
 * @param dst Destination directory (entries will be copied to here).
 * */
void sync_directories(const char *src, const char *dst);

/**
 * @brief Checks whether a file is considered temporary.
 *
 * Matches common temporary file patterns.
 *
 * @param filename Name of the file to check.
 * @return 1 if the file is temporary, 0 otherwise.
 **/
int is_temporary_file(const char *filename);

/**
 * @brief Check whether a file or directory is blacklisted.
 *
 * Reads the blacklist file from the path defined in the `BLACKLIST_PATH` environment variable and compares the entry.
 *
 * @return 1 if blacklisted, 0 otherwise.
 * @param black_dir Directory/file to be skipped
 **/
int blacklist(const char *black_dir);

#endif
