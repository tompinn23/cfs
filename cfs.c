#include "cfs.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define DIR_SEP '/'

#if defined(linux)
#define PLAT_DIR_SEP '/'
#elif defined(WIN32)
#define PLAT_DIR_SEP '\\'
#else
#define PLAT_DIR_SEP '/'
#endif

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

static char* cfs_strdup(const char* str);
static char* cfs_basename(char* str);

static cfs_fs_handler* find_handler(const char* filename) {
    cfs_fs_handler* cur = handlers;
    if(cur == NULL)
        return NULL;
	char* filename_dup = cfs_strdup(filename);
	char* basename = cfs_basename(filename_dup);	
	printf("%s\n", basename);
    char* extn = strrchr(basename, '.');
    if(extn == NULL) {
        extn = "";
    } else {
		extn++;
	}
	printf("%s\n", extn);
    do {
        const char** ext = cur->exts;
        while(*ext != NULL) {
            if(strcmp(extn, *ext) == 0) {
                // Found handler
                return cur;
            }
            ext++;
        }
        cur = cur->next;
    } while(cur->next);
    return NULL;
}

static char* cfs_strdup(const char* str) {
    size_t len = strlen(str);
    char* dup = cfs_malloc(sizeof(char) * (len + 1));
    dup[len] = '\0';
    memcpy(dup, str, len);
    return dup;
}

static char* cfs_basename(char* str) {
	int len = strlen(str);
	if(*(str + len) == PLAT_DIR_SEP)
		*(str + len) = '\0';
	return strrchr(str, PLAT_DIR_SEP) + 1;
}

static char* cfs_dirname(char* str) {
	int len = strlen(str);
	if(*(str + len) == '/')
		*(str + len) = '\0';
	char* base = strrchr(str, '/');
	if(base == NULL) 
		return ".";
	else {
		*base = '\0';
	}
	return str;
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
    cfs_mount_path* mnt = cfs_malloc(sizeof(cfs_mount_path));
    mnt->handle = handle;
    mnt->next = NULL;
	cfs_mount_path* mp = mount_path;
    if(mp != NULL) {
        while(mp->next) {
            mp = mp->next;
		}
		mp->next = mnt;
    } else {
		mount_path = mnt;
	}
    return 0;
}

cfs_file_handle* cfs_file_open(const char* filename, const char* mode) {
	cfs_mount_path* mp = mount_path;
	char* base = cfs_strdup(filename);
	char* basename = cfs_basename(base);
	printf("%s\n", basename);
	while(mp) {
		if(strstr(basename, mp->handle->mount) != NULL) {
		}
		mp = mp->next;
	}
	cfs_free(base);
}

long int cfs_file_read(cfs_file_handle* file, void* buffer, long int sz) {
    return 0;
}
