# OS Assignment 02 - Report

## Feature 2: Long Listing (v1.1.0)

**Q1. What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?**

`stat()` follows symbolic links: if the path is a symlink, it returns information about the file the link points to. `lstat()` does not follow the link and returns information about the link itself (its own type, size, permissions). In `ls`, `lstat()` is more appropriate because `ls -l` must show a symlink as a link (type `l`, with `-> target`) rather than showing the target's details. `lstat()` also avoids errors for broken (dangling) links, where `stat()` would fail.

**Q2. How can bitwise operators and macros extract file type and permissions from st_mode?**

`st_mode` packs the file type and the permission bits into one integer. The file type is extracted with the mask `S_IFMT`: `(st_mode & S_IFMT) == S_IFDIR` means a directory. The macros `S_ISDIR()`, `S_ISREG()`, `S_ISLNK()` do this check internally. Permission bits are tested with bitwise AND: `st_mode & S_IRUSR` is non-zero if the owner has read permission, similarly `S_IWUSR`, `S_IXUSR` for owner, `S_IRGRP`.. for group and `S_IROTH`.. for others. In my code, each bit decides whether `r`, `w`, `x` or `-` is printed. Special bits `S_ISUID`, `S_ISGID`, `S_ISVTX` change the execute character to `s`/`S`/`t`/`T`.

## Feature 3: Column Display (v1.2.0)

**Q1. Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the list insufficient?**

First all filenames are stored in an array and the longest name is found. Column width = longest name + 2 spaces. Number of columns = terminal width / column width, and number of rows = ceil(total files / columns). Printing goes row by row: for row `r` and column `c`, the item printed is `names[c * num_rows + r]`. A single loop printing names in array order would give an "across" layout (left to right), not "down then across", because consecutive names in the array must appear in the same column, one below another. So consecutive items of one row are `num_rows` apart in the array, and we need nested loops (rows outside, columns inside) with index arithmetic. It also needs total count and max length before printing.

**Q2. What is the purpose of ioctl here? What are the limitations of a fixed 80-column fallback?**

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` asks the terminal driver for the current window size (`ws_col` = number of columns), so the layout adapts to the real terminal width. With a fixed 80-column width, the output would waste space on wide terminals (fewer columns than possible) and would wrap and break the alignment on narrow terminals (lines longer than the screen). It also would not adapt when the user resizes the window.


## Feature 4: Horizontal Display -x (v1.3.0)

**Q1. Compare the implementation complexity of "down then across" (vertical) vs "across" (horizontal) printing. Which needs more pre-calculation and why?**

The vertical ("down then across") layout is more complex. Before printing anything it must know the total number of files and the longest name, then compute the number of columns (terminal width / column width) and the number of rows (ceil(count / columns)). Printing then needs nested loops with index arithmetic `names[c * num_rows + r]`, because the next item in the array goes below the current one, not beside it. The horizontal layout is simpler: it only needs the column width and the terminal width, and a single loop that prints each name padded to the column width while tracking the current horizontal position. When the next name would exceed the terminal width, it prints a newline and resets the position. It does not need the row count, so it needs less pre-calculation.

**Q2. Describe the strategy used to manage the different display modes (-l, -x, default). How did the program decide which function to call?**

I used an enum `DisplayMode` with three values: `MODE_DEFAULT`, `MODE_LONG` and `MODE_HORIZONTAL`. The `getopt()` loop in `main` sets this variable when it sees `-l` or `-x` (if both are given, the last one wins). The variable is passed to `do_ls()`. Inside `do_ls()`, if the mode is `MODE_LONG` it calls `do_ls_long()`; otherwise it reads all names into a dynamic array and then calls `display_horizontal()` for `MODE_HORIZONTAL`, or `display_columns()` (down then across) for the default mode.
