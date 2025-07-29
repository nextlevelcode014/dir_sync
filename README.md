# 🛡️ dir_sync – Directory Synchronization with Real-Time Monitoring

This project recursively **synchronizes directories** and monitors **changes in real time**, replicating updates from a source directory (`source`) to a target directory (`target`).
[See the documentation](https://nextlevelcode014.github.io/dir_sync)

[Quick demo video](https://youtu.be/BKlBIVOY54Q)

---

## 🔧 Features

- 🧠 Recursive synchronization of files and folders
- 🔁 Continuous monitoring using `inotify` 
- 📝 Color-coded logs with action descriptions (creation, modification, deletion)
- 🧹 Temporary and irrelevant files are ignored (e.g., `.swp`, `4913`, `~`, `.tmp`, `.git`, etc.)
- 🗂️ Support for multiple parallel instances for different directory pairs

---

## Notices
> ⚠️ **Warning**: Any file in the target directory will be overwritten if it matches a file in the source directory.

> ℹ️ **Note**: This tool only works on Linux.
---
## 🏗️ Compilation

You need a C compiler (like GCC). Just run:

```bash
make
````

Or, to clean and compile:

```bash
make clean && make
```

---

## ▶️ Execution

```bash
./dir_sync <source_dir> <target_dir>
```

Example:

```bash
./dir_sync ~/.config/nvim/ ~/dev/sysconfig/nvim/
```

> The target directory will be synchronized with the source and continuously monitored for new changes.

---

## 📜 Ignoring Temporary Files

Common temporary files ignored by default:

* `4913` (created by Vim)
* `*.swp`, `*.tmp` (swap and temp files)
* `*~` (backup files)
* Files starting with `.nfs` (NFS temp files) or `.#` (Emacs lock files)

The logic is implemented in `utils.c`:

```c
int is_temporary_file(const char *filename) {
    return (
        strcmp(filename, "4913") == 0 ||                       
        fnmatch("*.swp", filename, 0) == 0 ||                 
        fnmatch("*.swo", filename, 0) == 0 ||
        fnmatch("*.swn", filename, 0) == 0 ||
        fnmatch(".*.sw?", filename, 0) == 0 ||                

        fnmatch("#*#", filename, 0) == 0 ||                   
        fnmatch(".#*", filename, 0) == 0 ||                   

        fnmatch("*~", filename, 0) == 0 ||                     
        fnmatch("*.bak", filename, 0) == 0 ||                 
        fnmatch("*.tmp", filename, 0) == 0 ||                

        fnmatch("*.code-workspace.temp", filename, 0) == 0 ||
        fnmatch(".vscode", filename, 0) == 0 ||             

        fnmatch(".~lock.*#", filename, 0) == 0
    );
}
```

---

## 🖥️ Running with Multiple Directory Pairs

You can create a script like `dir_sync_all.sh`:

```bash
#!/bin/bash

BIN="$HOME/bin/dir_sync"

$BIN "$HOME/.config/i3/" "$HOME/dev/sysconfig/i3/" &
$BIN "$HOME/.config/nvim/" "$HOME/dev/sysconfig/nvim/" &
$BIN "$HOME/.config/fish/config.fish" "$HOME/dev/sysconfig/fish/config.fish" &
$BIN "$HOME/.config/starship.toml" "$HOME/dev/sysconfig/starship/starship.toml" &

wait
```

Grant execution permission:

```bash
chmod +x dir_sync_all.sh
```

And run the script:

```bash
./dir_sync_all.sh
```

---

## 🧩 Running as a systemd Service

Suggested structure:

```
mkdir -p ~/.local/dir_sync
cp dir_sync dir_sync_all.sh ~/.local/dir_sync/
chmod +x ~/.local/dir_sync/*
```

Create the following unit file:

```
~/.config/systemd/user/dir_sync.service
```

Content:

```ini
[Unit]
Description=Continuous file monitoring using dir_sync
After=network.target

[Service]
ExecStart=%h/.local/dir_sync/dir_sync_all.sh
Restart=on-failure
Environment=DISPLAY=:0
Environment=XAUTHORITY=%h/.Xauthority

[Install]
WantedBy=default.target
```

Reload and enable the service:

```bash
systemctl --user daemon-reexec
systemctl --user daemon-reload
systemctl --user enable --now dir_sync.service
```

Check if it's running:

```bash
systemctl --user status dir_sync.service
```

View logs:

```bash
journalctl --user -u dir_sync.service -f
```

---

## 📁 Project Structure

```
dir_sync/
├── main.c
├── config.c/.h
├── watcher.c/.h        # Handles inotify logic
├── utils.c/.h          # Filesystem and logging utilities
├── Makefile
├── dir_sync_all.sh     # Optional script to run multiple instances
└── README.md
```

---

## 🧩 Dependencies

* Linux
* Standard C library (glibc)
* `<sys/inotify.h>` header

---

## 📜 License

MIT — free to use, modify, and distribute.
