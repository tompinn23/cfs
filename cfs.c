#include "cfs.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct cfs_fs_handler {
    cfs_fs_impl* impl;
    const char** exts;
    void* userdata;
    struct cfs_fs_handler* next;
} cfs_fs_handler;

typedef struct cfs_mount_path {
    cfs_fs_handle* handle;
    struct cfs_mount_path* next;
} cfs_mount_path;

static int cfs_err;
static cfs_fs_handler* handlers;
static cfs_mount_path* mount_path;

static cfs_fs_handler* find_handler(const char* filename) {
    cfs_fs_handler* cur = handlers;
    if(cur == NULL)
        return NULL;
    char* extn = strrchr(filename, '.');
    if(extn == NULL) {
        extn = "";
    }
    while(cur->next) {
        const char** ext = cur->exts;
        while(*ext != NULL) {
            if(strcmp(extn + 1, *ext) == 0) {
                // Found handler
                return cur;
            }
            ext++;
        }
        cur = cur->next;
    }
    return NULL;
}

static char* cfs_strdup(const char* str) {
    size_t len = strlen(str);
    char* dup = cfs_malloc(sizeof(char) * (len + 1));
    dup[len] = '\0';
    memcpy(dup, str, len);
    return dup;
}
static const char* err = "";
const char* cfs_getstrerr(int errnum) {
    switch(errnum) {
    case CFS_ERRNOMEM:
        err = "Out of Memory";
        break;
    case CFS_ERRNOHANDLER:
        err = "No mount handler for filetype";
        break;
    }
    return err;
}


int cfs_fs_impl_register(cfs_fs_impl* impl, const char** extensions, void* userdata) {
    cfs_fs_handler* h = handlers;
    if(h != NULL) {
        while(h->next) {
            h = h->next;
        }
    }
    h = cfs_malloc(sizeof(cfs_fs_handler));
    if(h == NULL)
        return CFS_ERRNOMEM;
    h->impl = impl;
    h->exts = extensions;
    h->userdata = userdata;
    h->next = NULL;
    if(handlers == NULL) {
        handlers = h;
    }
}

int cfs_fs_mount(const char* src, const char* mount) {
    cfs_fs_handler* handler = find_handler(src);
    if(handler == NULL) {
        return CFS_ERRNOHANDLER;
    }
    cfs_fs_handle* handle = cfs_malloc(sizeof(cfs_fs_handle));
    handle->handler = handler;
    handle->src = cfs_strdup(src);
    handle->mount = cfs_strdup(mount);
    cfs_mount_path* mnt = mount_path;
    if(mnt != NULL) {
        while(mnt->next) {
            mnt = mnt->next;
        }
    }
    mnt = cfs_malloc(sizeof(cfs_mount_path));
    mnt->handle = handle;
    mnt->next = NULL;
    return 0;
}

cfs_file_handle* cfs_file_open(const char* filename, const char* mode) {
    //cfs_fs_handle* handler = find_handler(filename);
}

long int cfs_file_read(cfs_file_handle* file, void* buffer, long int sz) {
    return 0;
}
