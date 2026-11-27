# File Management System in C (Linux System Calls)

A menu-driven File Management System implemented in C using POSIX/Linux system calls (`open()`, `read()`, `write()`, `close()`, `unlink()`, `rename()`, `fstat()`, `stat()`, `chmod()`), executed on Windows 11 via Windows Terminal / WSL2 (Ubuntu).

## Features

1. **Create File** — Creates a strictly new file (`O_CREAT | O_EXCL | O_WRONLY`, mode `0644`).
2. **Write to File** — Overwrites an existing file (`O_WRONLY | O_TRUNC`) and displays updated file size via `fstat()`.
3. **Read File** — Reads and displays file contents via `read()` and `write(STDOUT_FILENO, ...)`.
4. **Append to File** — Appends content (`O_WRONLY | O_APPEND`) without overwriting existing data and displays updated file size via `fstat()`.
5. **Copy File** — Copies contents using buffered `read()`/`write()` with directory and self-copy (`st_dev`/`st_ino`) guards.
6. **Rename File** — Renames files atomically using `rename()`.
7. **Delete File** — Removes files using `unlink()`.
8. **Change File Permissions** — Inspects and updates octal permissions (`0644`, `0444`, `0755`, etc.) using `stat()` and `chmod()`.

## Build & Run (Windows PowerShell + WSL)

```powershell
wsl gcc -Wall -Wextra -Werror -std=c11 -o fms file_management_system.c
wsl ./fms
```

Or on Linux directly:

```bash
gcc -Wall -Wextra -Werror -std=c11 -o fms file_management_system.c
./fms
```

## Project Files

- `file_management_system.c` — Complete C implementation using Linux system calls.
- `ASSIGNMENT_REPORT.md` — Markdown report covering Parts A, B, and C with cropped screenshots.
- `ASSIGNMENT_REPORT.pdf` — Formatted PDF assignment submission with diagrams, complete code, and one-by-one cropped screenshots.
- `screenshots/` — Individual Windows PowerShell execution screenshots for every operation.
