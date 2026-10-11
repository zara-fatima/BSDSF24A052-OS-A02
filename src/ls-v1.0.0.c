/*
* Programming Assignment 02: lsv1.1.0
* Added: -l (long listing format)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

extern int errno;

void do_ls(const char *dir, int long_flag);
void do_ls_long(const char *dir);
void mode_to_string(mode_t mode, char *str);
const char *user_name(uid_t uid, char *buf);
const char *group_name(gid_t gid, char *buf);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], long_flag);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_flag)
{
    if (long_flag)
    {
        do_ls_long(dir);
        return;
    }

    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }
    if (errno != 0)
        perror("readdir failed");
    closedir(dp);
}

/* st_mode ko "drwxr-xr-x" jaisi string mein badalta hai */
void mode_to_string(mode_t mode, char *str)
{
    /* 1) file type */
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    /* 2) rwx bits: owner, group, others */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    /* 3) special bits: setuid, setgid, sticky */
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/* uid -> naam (agar naam na mile to number print) */
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

    /* PASS 1: har column ki sabse bari width nikalo (alignment ke liye) */
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

    printf("total %ld\n", total / 2); /* st_blocks 512-byte units mein hota hai */

    /* PASS 2: ab asal printing */
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

        /* ctime() = "Thu Oct 11 21:23:45 2026\n" -> 4th char se 12 chars = "Oct 11 21:23" */
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

        /* symbolic link ho to "-> target" bhi dikhao */
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
