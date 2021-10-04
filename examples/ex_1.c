#include "cfs.h"

#include <stdio.h>
#include <stdlib.h>

cfs_file_handle* stdio_open(cfs_fs_handle* handle, const char* filename, const char* mode) {
    cfs_file_handle* file = malloc(sizeof(cfs_file_handle));
    file->handle = fopen(filename, mode);
    file->fs_impl = handle;
    return file;
}

long int stdio_read(cfs_fs_handle* handle, cfs_file_handle* file, void* buffer, long int sz) {
    FILE* fp = (FILE*)file->handle;
    long int ret = fread(buffer, sz, sizeof(unsigned char), fp);
    return ret;
}

long int stdio_seek(cfs_fs_handle* handle, cfs_file_handle* file, long int offset, int whence) {
    FILE* fp = (FILE*)file->handle;
    int stdio_whence;
    switch(whence) {
        case CFS_SEEK_SET:
            stdio_whence = SEEK_SET;
            break;
        case CFS_SEEK_CUR:
            stdio_whence = SEEK_CUR;
            break;
        case CFS_SEEK_END:
            stdio_whence = SEEK_END;
            break;
    }
    return fseek(fp, offset, stdio_whence);
}

long int stdio_write(cfs_fs_handle* handle, cfs_file_handle* file, void* buffer, long int sz) {
    return 0;
}
static cfs_fs_impl stdio_impl = {
    .open_fn = stdio_open,
    .read_fn = stdio_read,
    .seek_fn = stdio_seek,
    .write_fn = stdio_write
};

static const char* exts[] = {
    "",
    NULL
};

int main(int argc, char** argv) {
    cfs_fs_impl_register(&stdio_impl, exts, NULL);
    int err = 0;
    if((err = cfs_fs_mount("./tmp", "/")) < 0) {
        printf("cfs error: %s\n", cfs_getstrerr(err));
        return -1;
    }
    cfs_file_handle* file = cfs_file_open("yolo.txt", "r");
    char buf[16];
    cfs_file_read(file, buf, 16);
    buf[15] = '\0';
    printf("%s", buf);
    return 0;
}
