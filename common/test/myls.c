#include <sys/types.h>
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void
print_file_type(struct stat *st)
{
    char c;

    switch (st->st_mode & S_IFMT) {
        case S_IFBLK:
            c = 'b';
            break;
        case S_IFCHR:
            c = 'c';
            break;
        case S_IFDIR:
            c = 'd';
            break;
        case S_IFIFO:
            c = 'p';
            break;
        case S_IFLNK:
            c = 'l';
            break;
        case S_IFREG:
            c = '-';
            break;
        case S_IFSOCK:
            c = 's';
            break;
        default:
            c = '?';
            break;
    }
    printf("%c", c);
}

void
print_perms(struct stat *st)
{
    printf("%c", st->st_mode & S_IRUSR ? 'r' : '-');
    printf("%c", st->st_mode & S_IWUSR ? 'w' : '-');
    printf("%c", st->st_mode & S_IXUSR ? 'x' : '-');
    printf("%c", st->st_mode & S_IRGRP ? 'r' : '-');
    printf("%c", st->st_mode & S_IWGRP ? 'w' : '-');
    printf("%c", st->st_mode & S_IXGRP ? 'x' : '-');
    printf("%c", st->st_mode & S_IROTH ? 'r' : '-');
    printf("%c", st->st_mode & S_IWOTH ? 'w' : '-');
    printf("%c", st->st_mode & S_IXOTH ? 'x' : '-');
}

void
print_mtime(struct stat *st)
{
    char          mt[32];
    struct tm     lt;
    time_t        t;

    t = st->st_mtime;
    localtime_r(&t, &lt);
    strftime(mt, sizeof mt, "%b %d %H:%M", &lt);
    printf("%s ", mt);
}

void
print_owner_group(struct stat *st)
{
    struct group  *grp;
    struct passwd *pwd;

    grp = getgrgid(st->st_gid);
    pwd = getpwuid(st->st_uid);
    printf("%-5s ", grp->gr_name);
    printf("%-5s ", pwd->pw_name);
}

int
main() {
    struct dirent *dir;
    DIR           *mydir;
    struct stat   st;

    mydir = opendir(".");
    while (1) {
        dir = readdir(mydir);
        if (dir == NULL) {
            break;
        }
        stat(dir->d_name, &st);
        print_file_type(&st);
        print_perms(&st);
        printf(" %d ", st.st_nlink);
        print_owner_group(&st);
        printf("%5ld ", st.st_size);
        print_mtime(&st);
        printf("%s\n", dir->d_name);
    }
}
