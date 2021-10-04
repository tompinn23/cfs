#ifndef __CFS_H__
#define __CFS_H__

#include <stdarg.h>

#ifndef cfs_malloc
    #define cfs_malloc(size) malloc(size)
#endif
#ifndef cfs_free
    #define cfs_free(ptr) free(ptr)
#endif
#ifndef cfs_realloc
    #define cfs_realloc(ptr, size) realloc(ptr, size)
#endif
typedef struct cfs_fs_handler cfs_fs_handler;
typedef struct cfs_fs_handle cfs_fs_handle;
typedef struct cfs_file_handle cfs_file_handle;
typedef struct cfs_fs_impl cfs_fs_impl;

typedef cfs_file_handle* (*cfs_fs_impl_open)(cfs_fs_handle* fs, const char* filename, const char* mode);
typedef long int (*cfs_fs_impl_seek)(cfs_fs_handle* fs, cfs_file_handle* handle, long int offset, int whence);
typedef long int (*cfs_fs_impl_read)(cfs_fs_handle* fs, cfs_file_handle* handle, void* buffer, long int sz);
typedef long int (*cfs_fs_impl_write)(cfs_fs_handle* fs, cfs_file_handle* handle, void* buffer, long int sz);

enum {
    CFS_SEEK_SET,
    CFS_SEEK_CUR,
    CFS_SEEK_END
};

enum {
    CFS_ERRNOMEM = -1,
    CFS_ERRNOHANDLER = -2,

};

/*
    Filesystem implementation struct to be filled by the user defined callbacks 
    for e.g. stdio / tar / zlib etc.
*/
typedef struct cfs_fs_impl {
    cfs_fs_impl_open* open_fn;
    cfs_fs_impl_seek* seek_fn;
    cfs_fs_impl_read* read_fn;
    cfs_fs_impl_write* write_fn;
} cfs_fs_impl;

typedef struct cfs_fs_handle {
    cfs_fs_handler* handler;
    const char* mount;
    const char* src;
    void* userdata;
} cfs_fs_handle;

typedef struct cfs_file_handle {
    void* handle;
    cfs_fs_handle* fs_impl;
} cfs_file_handle;

typedef struct cfs_file {

} cfs_file;

int cfs_fs_impl_register(cfs_fs_impl* impl, const char** extensions, void* userdata);
int cfs_fs_mount(const char* src, const char* mount);

const char* cfs_getstrerr(int errnum);
/*
    File handling functions mirrors stdio functions.
*/
int cfs_file_close(cfs_file* file);
void cfs_file_clear_error(cfs_file* file);
int cfs_file_eof(cfs_file* file);
int cfs_file_error(cfs_file* file);
int cfs_file_flush(cfs_file* file);
//int cfs_file_getpos(cfs_fs_file* file, long int* pos);

cfs_file_handle* cfs_file_open(const char* filename, const char* mode);
long int cfs_file_read(cfs_file_handle* file, void* buffer, long int sz);
long int cfs_file_write(cfs_file_handle* file, void* buffer, long int sz);

int cfs_file_fseek(cfs_file_handle* file, long int offset, int whence);
long int cfs_file_ftell(cfs_file_handle* file);


int cfs_file_fprintf(cfs_file_handle* file, const char* format, ...);
int cfs_file_vfprintf(cfs_file_handle* file, const char* format, va_list arg);

int cfs_file_fscanf(cfs_file_handle* file, const char* format, ...);



#endif