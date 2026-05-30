#include "../include/fs.h"
#include "../include/terminal.h"

static File files[MAX_FILES];

void fs_init() {
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = 0;
        files[i].data[0] = 0;
    }
}

static int fs_find(const char *name) {
    int i, j;
    for (i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) continue;
        int match = 1;
        for (j = 0; name[j] || files[i].name[j]; j++) {
            if (name[j] != files[i].name[j]) { match = 0; break; }
        }
        if (match) return i;
    }
    return -1;
}

static void str_copy(char *dst, const char *src, int max) {
    int i;
    for (i = 0; i < max - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

static int str_len(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

int fs_create(const char *name) {
    if (fs_find(name) >= 0) {
        print("Error: file already exists!");
        return -1;
    }
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].size = 0;
            files[i].data[0] = 0;
            str_copy(files[i].name, name, MAX_FILENAME);
            return i;
        }
    }
    print("Error: file system full!");
    return -1;
}

int fs_write(const char *name, const char *data) {
    int idx = fs_find(name);
    if (idx < 0) {
        print("Error: file not found!");
        return -1;
    }
    int len = str_len(data);
    if (len >= MAX_FILESIZE) len = MAX_FILESIZE - 1;
    str_copy(files[idx].data, data, MAX_FILESIZE);
    files[idx].size = len;
    return 0;
}

char* fs_read(const char *name) {
    int idx = fs_find(name);
    if (idx < 0) return 0;
    return files[idx].data;
}

int fs_delete(const char *name) {
    int idx = fs_find(name);
    if (idx < 0) {
        print("Error: file not found!");
        return -1;
    }
    files[idx].used = 0;
    files[idx].name[0] = 0;
    files[idx].data[0] = 0;
    files[idx].size = 0;
    return 0;
}

int fs_append(const char *name, const char *data) {
    int idx = fs_find(name);
    if (idx < 0) {
        print("Error: file not found!");
        return -1;
    }
    // find end of current content
    int current_len = files[idx].size;
    int i = 0;
    while (data[i] && current_len + i < MAX_FILESIZE - 2) {
        files[idx].data[current_len + i] = data[i];
        i++;
    }
    // add space separator
    files[idx].data[current_len + i] = ' ';
    files[idx].data[current_len + i + 1] = 0;
    files[idx].size = current_len + i + 1;
    return 0;
}


void fs_list() {
    int i, found = 0;
    char sizebuf[12];
    for (i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) continue;
        found = 1;
        // print filename
        print("  ");
        print(files[i].name);
        // print size
        int s = files[i].size;
        int j = 0;
        char tmp[12];
        if (s == 0) { tmp[j++] = '0'; }
        while (s > 0) { tmp[j++] = '0' + (s % 10); s /= 10; }
        // reverse
        int k;
        for (k = 0; k < j / 2; k++) {
            char t = tmp[k]; tmp[k] = tmp[j-1-k]; tmp[j-1-k] = t;
        }
        tmp[j] = 0;
        print("  (");
        print(tmp);
        print(" bytes)\n");
    }
    if (!found) print("  (no files)");
}
