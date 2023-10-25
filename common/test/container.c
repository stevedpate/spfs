#include <stdio.h>

#define container_of(ptr, type, member) ({              \
    void *__mptr = (void *)(ptr);                   \
    ((type *)(__mptr - offsetof(type, member))); })

struct embedded {
    char            c[16];
} inside;

struct container {
    char            a[16];
    struct embedded b;
} outside;

int
main()
{
    struct container bigger_box;

    strcpy(bigger_box.a, "Hello ");
    strcpy(bigger_box.a, "Hello ");

    bigger_box = container_of(&inside, struct container, y);

    printf("%s\n", xbigger_box->, bigger_box->y);

}
