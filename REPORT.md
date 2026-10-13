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


## Feature 5: Alphabetical Sort (v1.4.0)

**Q1. Why is it necessary to read all directory entries into memory before sorting? What are the drawbacks for directories with millions of files?**

`readdir()` returns entries one at a time in whatever order the filesystem stores them, which is not alphabetical. To sort, the program must compare every entry with others, so all names must be available at the same time. A sorting algorithm cannot place an entry correctly until it has seen all entries (the first name returned could belong at the very end). The drawback with millions of files is high memory use (every name is a separate heap string plus a pointer in the array), a delay before the first line is printed (nothing can be shown until reading and sorting finish), and extra CPU time for sorting (O(n log n) comparisons). Reallocations of the growing array also add cost.

**Q2. Explain the purpose and signature of the comparison function required by qsort(). How does it work, and why must it take const void * arguments?**

`qsort()` is a generic function that does not know the type of the elements it sorts, so it needs a comparison function to decide the order of two elements. The signature is `int compare(const void *a, const void *b)`. It returns a negative number if the first element should come before the second, zero if they are equal, and a positive number if the first should come after. In my code the array holds `char *`, so `qsort` passes pointers to the elements, i.e. `char **`. The function casts `a` and `b` to `const char **`, dereferences them to get the strings and returns `strcmp(s1, s2)`. The parameters are `const void *` because `void *` can hold a pointer to any type, which lets one `qsort` work for ints, structs or strings, and `const` promises that the comparison will not modify the data.


## Feature 6: Colorized Output (v1.5.0)

**Q1. How do ANSI escape codes work to produce color in a Linux terminal? Show the code sequence for printing text in green.**

ANSI escape codes are special character sequences that start with the ESC character (octal `\033`, hex `0x1B`) followed by `[`, one or more numbers and a final letter. The terminal does not print them; it interprets them as formatting commands. The final letter `m` means "set graphics mode", and the numbers select the style: `0` is reset, `7` is reverse video, `31` is red, `32` is green, `34` is blue, `35` is magenta. To print green text: `printf("\033[0;32mHello\033[0m\n");`. The first sequence `\033[0;32m` switches the color to green, then the text is printed, and `\033[0m` resets the color to the default so the following output is not colored.

**Q2. To color an executable file, which bits in st_mode must be checked to determine if a file is executable by the owner, group or others?**

The execute permission bits of `st_mode` are `S_IXUSR` (0100) for the owner, `S_IXGRP` (0010) for the group and `S_IXOTH` (0001) for others. A file is executable if any of them is set: `st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)` is non-zero. In my code this check is only done for regular files (`S_ISREG`), because directories also have the execute (search) bit set but must be colored blue, not green.


## Feature 7: Recursive Listing -R (v1.6.0)

**Q1. In a recursive function, what is a "base case"? What is the base case that stops the recursion in your recursive ls?**

A base case is the condition under which a recursive function stops calling itself and returns directly; without it the function would call itself forever. In my recursive `ls`, the base case is a directory that contains no subdirectories (or one that cannot be opened). For such a directory, the loop that checks each entry with `lstat()` and `S_ISDIR()` never finds a directory to descend into, so no further `do_ls()` call is made and the function simply prints its entries and returns to the caller. Because every recursive call goes one level deeper into a finite directory tree, the recursion always ends. Skipping `.` and `..` is also essential, since otherwise a directory would call itself endlessly. Using `lstat()` means symbolic links are not followed as directories, which avoids infinite loops from link cycles.

**Q2. Why is it essential to construct a full path before making a recursive call? What would happen if you simply called do_ls("subdir") from within do_ls("parent_dir")?**

`readdir()` returns only the entry name (e.g. `subdir`), not its location. All file system calls (`opendir`, `lstat`) interpret a relative name starting from the current working directory of the process, not from the directory being listed. If I called `do_ls("subdir")` from inside `do_ls("parent_dir")`, the program would look for `./subdir` in the directory where it was started. Either that path does not exist, so `opendir` fails with "Cannot open directory" and the subdirectory is never listed, or, worse, a different directory with the same name in the current directory would be listed by mistake. Building the full path `parent_dir/subdir` (with `build_path()`) makes the call independent of the current working directory and always points to the correct directory. It also makes the printed header (e.g. `parent_dir/subdir:`) correct.
