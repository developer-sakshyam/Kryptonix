#ifndef FS_H
#define FS_H

#define MAX_FILES     16
#define MAX_FILENAME  32
#define MAX_FILESIZE  512

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILESIZE];
    int  size;
    int  used;
} File;

void     fs_init();
int      fs_create(const char *name);
int      fs_write(const char *name, const char *data);
char*    fs_read(const char *name);
int      fs_delete(const char *name);
int      fs_append(const char *name, const char *data);
void     fs_list();

#endif
