/*
* Programming Assignment 02: lsv1.3.0
* Added: -x (horizontal "across" column display)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

extern int errno;

/* Display mode ka state: koi ek mode hi active hota hai */
typedef enum
{
    MODE_DEFAULT,    /* down then across */
    MODE_LONG,       /* -l */
    MODE_HORIZONTAL  /* -x */
} DisplayMode;

void do_ls(const char *dir, DisplayMode mode);
void do_ls_long(const char *dir);
void display_columns(char **names, int count, int maxlen);
void display_horizontal(char **names, int count, int maxlen);
int get_terminal_width(void);
void mode_to_string(mode_t mode, char *str);
const char *user_name(uid_t uid, char *buf);
const char *group_name(gid_t gid, char *buf);

int main(int argc, char *argv[])
{
    int opt;
    DisplayMode mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [dir...]\n", argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

/* Terminal ki width (columns mein) maloom karo */
int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80; /* fallback: ioctl fail ho (jaise pipe) to 80 */
    return w.ws_col;
}

void do_ls(const char *dir, DisplayMode mode)
{
    /* long listing apna kaam khud karti hai */
    if (mode == MODE_LONG)
    {
        do_ls_long(dir);
        return;
    }

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* Saare naam dynamic array mein jama karo */
    int capacity = 64;
    int count = 0;
    int maxlen = 0;
    char **names = malloc(capacity * sizeof(char *));
    if (names == NULL)
    {
        perror("malloc failed");
        closedir(dp);
        return;
    }

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        /* array bhar jaye to double kar do */
        if (count == capacity)
        {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc failed");
                break;
            }
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        if (names[count] == NULL)
        {
            perror("strdup failed");
            break;
        }

        int len = (int)strlen(names[count]);
        if (len > maxlen)
            maxlen = len;
        count++;
    }
    if (errno != 0)
        perror("readdir failed");
    closedir(dp);

    /* Flag dekh kar sahi display function chunna */
    if (mode == MODE_HORIZONTAL)
        display_horizontal(names, count, maxlen);
    else
        display_columns(names, count, maxlen);

    /* memory wapas do */
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* "Down then across" printing (default mode) */
void display_columns(char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int term_width = get_terminal_width();
    int col_width = maxlen + 2; /* 2 spaces columns ke darmiyan */
    int num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;
    if (num_cols > count)
        num_cols = count;
    int num_rows = (count + num_cols - 1) / num_cols; /* ceiling division */

    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_cols; c++)
        {
            int idx = c * num_rows + r; /* column-major index */
            if (idx >= count)
                break;

            int is_last = (c == num_cols - 1) || (idx + num_rows >= count);
            if (is_last)
                printf("%s", names[idx]);
            else
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

/* "Across" printing (-x): left se right, line bhar jaye to neeche */
void display_horizontal(char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int term_width = get_terminal_width();
    int col_width = maxlen + 2;
    int pos = 0; /* current line par ab tak kitni jagah use hui */

    for (int i = 0; i < count; i++)
    {
        /* agar ye naam is line mein na samaye to nayi line shuru karo */
        if (pos > 0 && pos + col_width > term_width)
        {
            printf("\n");
            pos = 0;
        }
        printf("%-*s", col_width, names[i]);
        pos += col_width;
    }
    printf("\n");
}

/* st_mode ko "drwxr-xr-x" jaisi string mein badalta hai */
void mode_to_string(mode_t mode, char *str)
{
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

const char *user_name(uid_t uid, char *buf)
{
    struct passwd *pw = getpwuid(uid);
    if (pw) return pw->pw_name;
    snprintf(buf, 32, "%u", (unsigned)uid);
    return buf;
}

const char *group_name(gid_t gid, char *buf)
{
    struct group *gr = getgrgid(gid);
    if (gr) return gr->gr_name;
    snprintf(buf, 32, "%u", (unsigned)gid);
    return buf;
}

void do_ls_long(const char *dir)
{
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX];
    char ub[32], gb[32], nb[32];
    int w_link = 0, w_user = 0, w_group = 0, w_size = 0;
    long total = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
        {
            perror(path);
            continue;
        }
        total += st.st_blocks;

        int len = snprintf(nb, sizeof(nb), "%ld", (long)st.st_nlink);
        if (len > w_link) w_link = len;
        len = (int)strlen(user_name(st.st_uid, ub));
        if (len > w_user) w_user = len;
        len = (int)strlen(group_name(st.st_gid, gb));
        if (len > w_group) w_group = len;
        len = snprintf(nb, sizeof(nb), "%lld", (long long)st.st_size);
        if (len > w_size) w_size = len;
    }

    printf("total %ld\n", total / 2);

    rewinddir(dp);
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &st) == -1)
            continue;

        char perms[11];
        mode_to_string(st.st_mode, perms);

        char timebuf[13];
        char *t = ctime(&st.st_mtime);
        strncpy(timebuf, t + 4, 12);
        timebuf[12] = '\0';

        printf("%s %*ld %-*s %-*s %*lld %s %s",
               perms,
               w_link, (long)st.st_nlink,
               w_user, user_name(st.st_uid, ub),
               w_group, group_name(st.st_gid, gb),
               w_size, (long long)st.st_size,
               timebuf,
               entry->d_name);

        if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t n = readlink(path, target, sizeof(target) - 1);
            if (n != -1)
            {
                target[n] = '\0';
                printf(" -> %s", target);
            }
        }
        printf("\n");
    }
    closedir(dp);
}
