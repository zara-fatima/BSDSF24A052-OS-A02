# OS Assignment 02 - Report

## Feature 2: Long Listing (v1.1.0)

**Q1. What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?**

`stat()` follows symbolic links: if the path is a symlink, it returns information about the file the link points to. `lstat()` does not follow the link and returns information about the link itself (its own type, size, permissions). In `ls`, `lstat()` is more appropriate because `ls -l` must show a symlink as a link (type `l`, with `-> target`) rather than showing the target's details. `lstat()` also avoids errors for broken (dangling) links, where `stat()` would fail.

**Q2. How can bitwise operators and macros extract file type and permissions from st_mode?**

`st_mode` packs the file type and the permission bits into one integer. The file type is extracted with the mask `S_IFMT`: `(st_mode & S_IFMT) == S_IFDIR` means a directory. The macros `S_ISDIR()`, `S_ISREG()`, `S_ISLNK()` do this check internally. Permission bits are tested with bitwise AND: `st_mode & S_IRUSR` is non-zero if the owner has read permission, similarly `S_IWUSR`, `S_IXUSR` for owner, `S_IRGRP`.. for group and `S_IROTH`.. for others. In my code, each bit decides whether `r`, `w`, `x` or `-` is printed. Special bits `S_ISUID`, `S_ISGID`, `S_ISVTX` change the execute character to `s`/`S`/`t`/`T`.
