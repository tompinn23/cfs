#include "cfs.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>


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
static char* cfs_strnstr(char* haystack, char* needle, size_t len);

static cfs_fs_handler* find_handler(const char* filename) {
    cfs_fs_handler* cur = handlers;
    if(cur == NULL)
        return NULL;
	const char* extn;
	size_t len;
	cfs_path_extension(filename, &extn, &len);
	if(len == 0) {
		extn = "";
		len = 1;
	}
	printf("%.*s\n", len, extn);
    do {
        const char** ext = cur->exts;
        while(*ext != NULL) {
            if(strncmp(extn, *ext, len) == 0) {
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

static const char* err = "";
const char* cfs_getstrerr(int errnum) {
    switch(errnum) {
    case CFS_ERRNOMEM:
        err = "Out of Memory";
        break;
    case CFS_ERRNOHANDLER:
        err = "No mount handler for filetype";
        break;
	case CFS_ERRPATH:
		err = "Unexpected path error";
		break;
	default:
		err = "Unknown error";
		break;
    }
    return err;
}


int cfs_fs_impl_register(cfs_fs_impl* impl, const char** extensions, void* userdata) {
    cfs_fs_handler* handler = handlers;
    if(handler != NULL) {
        while(handler->next) {
            handler = handler->next;
        }
    }
    cfs_fs_handler* h = cfs_malloc(sizeof(cfs_fs_handler));
    if(h == NULL)
        return CFS_ERRNOMEM;
    h->impl = impl;
    h->exts = extensions;
    h->userdata = userdata;
    h->next = NULL;
	if(handlers != NULL) {
		handler = h;
	} else {
		handlers = h;
	}
	return 0;
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
	cfs_file_handle* file = NULL;
	cfs_mount_path* mp = mount_path;
	char* base = cfs_strdup(filename);
	size_t len;
	cfs_path_dirname(filename, &len);
	if(len == 0) {
		printf("NO dirname\n");
		return NULL;
	}
	base[len] = '\0';
	printf("file: %s, base: %s\n", filename, base);

	while(mp) {
		if(cfs_path_get_intersection(base, mp->handle->mount) != 0) {
			file = mp->handle->handler->impl->open_fn(mp->handle, filename, mode);
			if(file != NULL) {
				break;
			}
		}
		mp = mp->next;
	}
	cfs_free(base);
	return file;
}

long int cfs_file_read(cfs_file_handle* file, void* buffer, long int sz) {
    return 0;
}



/*
 * Path handling functions mostly taken from cwalk https://github.com/likle/cwalk
 *  and modified the API for platform and non platform specific versions.
 *
 */

#if defined(WIN32) || defined(_WIN32) ||                                       \
  defined(__WIN32) && !defined(__CYGWIN__)
static enum cfs_path_style path_style = CFS_PATH_WINDOWS;
#else
static enum cfs_path_style path_style = CFS_PATH_UNIX;
#endif

static const char* seperators[] = {
	"\\/",
	"/"
};

static bool cfs_path_get_first_segment_without_root(cfs_path_style style, const char* path, const char* segments, cfs_path *segment);
static bool cfs_path_get_first_segment_impl(cfs_path_style style, const char *path, cfs_path *segment);
static bool cfs_path_get_last_segment_impl(cfs_path_style style, const char* path, cfs_path* segment);
static bool cfs_path_get_next_segment_impl(cfs_path_style style, cfs_path* segment);
static bool cfs_path_get_previous_segment_impl(cfs_path_style style, cfs_path* segment);

static size_t cfs_path_normalize_impl(cfs_path_style style, const char* path, char* buffer, size_t buffer_size);

static void cfs_path_basename_impl(cfs_path_style style, const char* path, const char** basename, size_t* length);
static void cfs_path_dirname_impl(cfs_path_style style, const char* path, size_t* length);

static bool cfs_path_extension_impl(cfs_path_style style, const char* path, const char** extension, size_t* length);



static const char* cfs_path_find_next_stop(cfs_path_style style, const char* c);

void cfs_path_basename(const char* path, const char** basename, size_t* length) {
	cfs_path_basename_impl(CFS_PATH_UNIX, path, basename, length);
}

void cfs_path_dirname(const char *path, size_t *length) {
	cfs_path_dirname_impl(CFS_PATH_UNIX, path, length);
}

void cfs_plat_path_basename(const char* path, const char** basename, size_t* length) {
	cfs_path_basename_impl(path_style, path, basename, length);
}

void cfs_plat_path_dirname(const char *path, size_t *length) {
	cfs_path_dirname_impl(path_style, path, length);
}

static void cfs_path_basename_impl(cfs_path_style style, const char* path, const char** basename, size_t* length) {
	cfs_path segment;
	if(!cfs_path_get_last_segment_impl(style, path, &segment)) {
		*basename = NULL;
		*length = 0;
		return;
	}
	*basename = segment.begin;
	*length = segment.size;
}

static void cfs_path_dirname_impl(cfs_path_style style, const char* path, size_t* length) {
	cfs_path segment;
	if(!cfs_path_get_last_segment_impl(style, path, &segment)) {
		*length = 0;
		return;
	}
	*length = (size_t)(segment.begin - path);
}

static bool cfs_path_extension_impl(cfs_path_style style, const char* path, const char** extension, size_t* length) {
	cfs_path segment;
	const char* c;

	if(!cfs_path_get_last_segment_impl(style, path, &segment)) {
		return false;
	}

	for(c = segment.end; c >= segment.begin; --c) {
		if(*c == '.') {
			*extension = c;
			*length = (size_t)(segment.end - c);
			return true;
		}
	}
	return false;
}

bool cfs_path_extension(const char *path, const char **extension, size_t *length) {
	return cfs_path_extension_impl(CFS_PATH_UNIX, path, extension, length);
}

bool cfs_plat_path_extension(const char *path, const char **extension, size_t *length) {
	return cfs_path_extension_impl(path_style, path, extension, length);
}

bool cfs_path_has_extension(const char *path) {
	const char* extension;
	size_t length;

	return cfs_path_extension_impl(CFS_PATH_UNIX, path, &extension, &length);
}

bool cfs_plat_path_has_extension(const char *path) {
	const char* extension;
	size_t length;

	return cfs_path_extension_impl(path_style, path, &extension, &length);
}

bool cfs_path_is_sep(cfs_path_style style, const char* str) {
	const char* c;
	c = seperators[style];
	while(*c) {
		if(*c == *str) { return true; }
		++c;
	}
	return false;
}

size_t cfs_path_normalize(const char *path, char *buffer, size_t buffer_size) {
	return cfs_path_normalize_impl(CFS_PATH_UNIX, path, buffer, buffer_size);
}

size_t cfs_plat_path_normalize(const char *path, char *buffer, size_t buffer_size) {
	return cfs_path_normalize_impl(path_style, path, buffer, buffer_size);
}


static void cfs_path_get_root_windows(const char* path, size_t* length) {
	const char *c;
	bool is_device_path;

	// We can not determine the root if this is an empty string. So we set the
	// root to NULL and the length to zero and cancel the whole thing.
  	c = path;
  	*length = 0;
  	if (!*c) {
    	return;
	}

  	// Now we have to verify whether this is a windows network path (UNC), which
  	// we will consider our root.
  	if (cfs_path_is_sep(CFS_PATH_WINDOWS, c)) {
  	  ++c;
	}
    // Check whether the path starts with a single back slash, which means this
    // is not a network path - just a normal path starting with a backslash.
    if (!cfs_path_is_sep(CFS_PATH_WINDOWS, c)) {
      // Okay, this is not a network path but we still use the backslash as a
      // root.
      ++(*length);
      return;
    }

    // A device path is a path which starts with "\\." or "\\?". A device path
    // can be a UNC path as well, in which case it will take up one more
    // segment. So, this is a network or device path. Skip the previous
    // separator. Now we need to determine whether this is a device path. We
    // might advance one character here if the server name starts with a '?' or
    // a '.', but that's fine since we will search for a separator afterwards
    // anyway.
    ++c;
    is_device_path = (*c == '?' || *c == '.') && cfs_path_is_sep(CFS_PATH_WINDOWS, ++c);
    if (is_device_path) {
      // That's a device path, and the root must be either "\\.\" or "\\?\"
      // which is 4 characters long. (at least that's how Windows
      // GetFullPathName behaves.)
      *length = 4;
      return;
    }

    // We will grab anything up to the next stop. The next stop might be a '\0'
    // or another separator. That will be the server name.
    c = cfs_path_find_next_stop(CFS_PATH_WINDOWS, c);

    // If this is a separator and not the end of a string we wil have to include
    // it. However, if this is a '\0' we must not skip it.
    while (cfs_path_is_sep(CFS_PATH_WINDOWS, c)) {
      ++c;
    }

    // We are now skipping the shared folder name, which will end after the
    // next stop.
    c = cfs_path_find_next_stop(CFS_PATH_WINDOWS, c);

    // Then there might be a separator at the end. We will include that as well,
    // it will mark the path as absolute.
    if (cfs_path_is_sep(CFS_PATH_WINDOWS, c)) {
      ++c;
    }

    // Finally, calculate the size of the root.
    *length = (size_t)(c - path);
    return;
}

static const char* cfs_path_find_next_stop(cfs_path_style style, const char* c) {
	while(*c != '\0' && !cfs_path_is_sep(style, c)) {
		++c;
	}
	return c;
}

static void cfs_path_get_root_unix(const char* path, size_t* length) {
	if(cfs_path_is_sep(CFS_PATH_UNIX, path)) {
		*length = 1;
	} else {
		*length = 0;
	}
}


void cfs_path_get_root(cfs_path_style style, const char *path, size_t *length) {
	switch(style) {
		case CFS_PATH_WINDOWS:
			cfs_path_get_root_windows(path, length);
		break;
		case CFS_PATH_UNIX:
			cfs_path_get_root_unix(path, length);
		break;
	}
}


bool cfs_path_get_first_segment(const char *path, cfs_path *segment) {
	return cfs_path_get_first_segment_impl(CFS_PATH_UNIX, path, segment);
}

bool cfs_plat_path_get_first_segment(const char* path, cfs_path* segment) {
	return cfs_path_get_first_segment_impl(path_style, path, segment);
}

static bool cfs_path_get_first_segment_impl(cfs_path_style style, const char* path, cfs_path* segment) {
	size_t length;
	const char* segments;
	cfs_path_get_root(style, path, &length);
	segments = path + length;

	return cfs_path_get_first_segment_without_root(style, path, segments, segment);
}
static bool cfs_path_get_first_segment_without_root(cfs_path_style style, const char* path, const char* segments, cfs_path *segment) {
	segment->path = path;
	segment->segments = segments;
	segment->begin = segments;
	segment->end = segments;
	segment->size = 0;

	if(*segments == '\0') {
		return false;
	}

	while(cfs_path_is_sep(style, segments)) {
		++segments;
		if(*segments == '\0') {
			return false;
		}
	}

	segment->begin = segments;
	segments = cfs_path_find_next_stop(style, segments);

	segment->size = (size_t)(segments - segment->begin);
	segment->end = segments;
	return true;
}

static bool cfs_path_get_last_segment_impl(cfs_path_style style, const char* path, cfs_path* segment) {
	if(!cfs_path_get_first_segment_impl(style, path, segment)) {
		return false;
	}

	while(cfs_path_get_next_segment_impl(style, segment)) {}

	return true;
}

bool cfs_path_get_last_segment(const char *path, cfs_path *segment) {
	return cfs_path_get_last_segment_impl(CFS_PATH_UNIX, path, segment);
}

bool cfs_plat_path_get_last_segment(const char* path, cfs_path* segment) {
	return cfs_path_get_last_segment_impl(path_style, path, segment);
}

static bool cfs_path_get_next_segment_impl(cfs_path_style style, cfs_path* segment) {
	const char* c;
	c = segment->begin + segment->size;
	if(*c == '\0') {
		return false;
	}

	assert(cfs_path_is_sep(style, c));
	do {
		++c;
	} while(cfs_path_is_sep(style, c));
	
	if(*c == '\0')
		return false;

	segment->begin = c;
	c = cfs_path_find_next_stop(style, c);
	segment->end = c;
	segment->size = (size_t)(c - segment->begin);

	return true;
}

bool cfs_path_get_next_segment(cfs_path *segment) {
	return cfs_path_get_next_segment_impl(CFS_PATH_UNIX, segment);
}

bool cfs_plat_path_get_next_segment(cfs_path *segment) {
	return cfs_path_get_next_segment_impl(path_style, segment);
}

static const char* cfs_path_find_previous_stop(cfs_path_style style, const char* begin, const char* c) {
	while (c > begin && !cfs_path_is_sep(style, c)) {
		--c;
	}

	if(cfs_path_is_sep(style, c)) {
		return c + 1;
	} else {
		return c;
	}

}

static bool cfs_path_get_previous_segment_impl(cfs_path_style style, cfs_path* segment) {
	const char* c;

	c = segment->begin;
	if(c <= segment->segments) {
		return false;
	}

	do {
		--c;
		if(c < segment->segments) {
			return false;
		}
	} while(cfs_path_is_sep(style, c));

	segment->end = c + 1;
	segment->begin = cfs_path_find_previous_stop(style, segment->segments, c);
	segment->size = (size_t)(segment->end - segment->begin);

	return true;
}



typedef struct cfs_segment_joined {
	cfs_path segment;
	const char** paths;
	size_t path_index;
} cfs_segment_joined;

static bool cfs_path_string_equal(cfs_path_style style, const char* first, const char* second, size_t first_size, size_t second_size) {
	if(first_size != second_size) {
		return false;
	}

	if(style == CFS_PATH_UNIX) {
		return strncmp(first, second, first_size) == 0;
	}

	while(*first && *second && first_size > 0) {
		if(tolower(*first++) != tolower(*second++)) {
			return false;
		}
		--first_size;
	}
	return true;
}

static bool cfs_path_get_first_segment_joined(cfs_path_style style, const char** paths, cfs_segment_joined* sj) {
	bool result;

	sj->path_index = 0;
	sj->paths = paths;
	result = false;
  	while (paths[sj->path_index] != NULL &&
         (result = cfs_path_get_first_segment_impl(style, paths[sj->path_index],
            &sj->segment)) == false) {
    ++sj->path_index;
  }

  return result;
}

static bool cfs_path_is_root_absolute(cfs_path_style style, const char* path, size_t length) {
	if(length == 0) {
		return false;
	}

	return cfs_path_is_sep(style, &path[length -1]);
}

static enum cfs_path_segment_type cfs_path_get_segment_type(const cfs_path* segment) {
	if(strncmp(segment->begin, ".", segment->size) == 0) {
		return CFS_PATH_CURRENT;
	} else if(strncmp(segment->begin, "..", segment->size) == 0) {
		return CFS_PATH_BACK;
	}
	return CFS_PATH_NORMAL;
}

static bool cfs_path_get_last_segment_without_root(cfs_path_style style, const char* path, cfs_path* segment) {
	if(!cfs_path_get_first_segment_without_root(style, path, path, segment)) {
		return false;
	}

	while(cfs_path_get_next_segment_impl(style, segment)) {}

	return true;
}



static bool cfs_path_get_previous_segment_joined(cfs_path_style style, cfs_segment_joined* sj) {
	bool result;

	if(*sj->paths == NULL) {
		return false;
	} else if(cfs_path_get_previous_segment_impl(style, &sj->segment)) {
		return true;
	}


	result = false;
	do {
		// We are done once we reached index 0. In that case there are no more
		// segments left.
    
		if (sj->path_index == 0) {
			break;
		}

    	// There is another path which we have to inspect. So we decrease the path
		// index.
	    --sj->path_index;
		
    	// If this is the first path we will have to consider that this path might
    	// include a root, otherwise we just treat is as a segment.
    	if (sj->path_index == 0) {
      		result = cfs_path_get_last_segment_impl(style, sj->paths[sj->path_index],
        	&sj->segment);
    	} else {
      		result = cfs_path_get_last_segment_without_root(style, sj->paths[sj->path_index],
        	&sj->segment);
    	}
	} while (!result);

	return result;	
}



static bool cfs_path_segment_back_will_be_removed(cfs_path_style style, cfs_segment_joined* sj) {
	enum cfs_path_segment_type type;
	int counter;
	
	counter = 0;

	while(cfs_path_get_previous_segment_joined(style, sj)) {
		type = cfs_path_get_segment_type(&sj->segment);
		if(type == CFS_PATH_NORMAL) {
			++counter;
			if(counter > 0) {
				return true;
			}
		} else if (type == CFS_PATH_BACK) {
			--counter;
		}
	}

	return false;
}

static bool cfs_path_get_next_segment_joined(cfs_path_style style, cfs_segment_joined *sj) {
	bool result;

	if(sj->paths[sj->path_index] == NULL) {
		return false;
	} else if(cfs_path_get_next_segment_impl(style, &sj->segment)) {
		return true;
	}

	result = false;
	do {
		++sj->path_index;

		if(sj->paths[sj->path_index] == NULL) {
			break;
		}

		result = cfs_path_get_first_segment_without_root(style, sj->paths[sj->path_index], sj->paths[sj->path_index], &sj->segment);
	} while(!result);

	return result;
}

static bool cfs_path_segment_normal_will_be_removed(cfs_path_style style, cfs_segment_joined* sj) {
	enum cfs_path_segment_type type;
	int counter;

	counter = 0;

	while(cfs_path_get_next_segment_joined(style, sj)) {
		type = cfs_path_get_segment_type(&sj->segment);
		if(type == CFS_PATH_NORMAL) {
			++counter;
		} else if(type == CFS_PATH_BACK) {
			--counter;
			if(counter < 0) {
				return true;
			}
		}
	}
	return false;
}



static bool cfs_path_segment_will_be_removed(cfs_path_style style, const cfs_segment_joined* sj, bool absolute) {
	enum cfs_path_segment_type type;
	cfs_segment_joined sjc;
	
	sjc = *sj;

	type = cfs_path_get_segment_type(&sj->segment);
	if(type == CFS_PATH_CURRENT || (type == CFS_PATH_BACK && absolute)) {
		return true;
	} else if(type == CFS_PATH_BACK) {
		return cfs_path_segment_back_will_be_removed(style, &sjc);
	} else {
		return cfs_path_segment_normal_will_be_removed(style, &sjc);
	}
}

static bool cfs_path_segment_joined_skip_invisible(cfs_path_style style, cfs_segment_joined* sj, bool absolute) {
	while(cfs_path_segment_will_be_removed(style, sj, absolute)) {
		if(!cfs_path_get_next_segment_joined(style, sj)) {
			return false;
		}
	}

	return true;
}

static size_t cfs_path_get_intersection_impl(cfs_path_style style, const char* path_base, const char* path_other) {
	bool absolute;
	size_t base_root_length, other_root_length;
	const char *end;
	const char *paths_base[2], *paths_other[2];
	cfs_segment_joined base, other;
	
  	// We first compare the two roots. We just return zero if they are not equal.
  	// This will also happen to return zero if the paths are mixed relative and
  	// absolute.
	cfs_path_get_root(style, path_base, &base_root_length);
	cfs_path_get_root(style, path_other, &other_root_length);
	if (!cfs_path_string_equal(style, path_base, path_other, base_root_length, other_root_length)) {
		return 0;
	}

 	// Configure our paths. We just have a single path in here for now.
 	paths_base[0] = path_base;
  	paths_base[1] = NULL;
  	paths_other[0] = path_other;
  	paths_other[1] = NULL;

  	// So we get the first segment of both paths. If one of those paths don't have
  	// any segment, we will return 0.
  	if (!cfs_path_get_first_segment_joined(style, paths_base, &base) ||
    	  !cfs_path_get_first_segment_joined(style, paths_other, &other)) {
    	return base_root_length;
  	}

  	// We now determine whether the path is absolute or not. This is required
  	// because if will ignore removed segments, and this behaves differently if
  	// the path is absolute. However, we only need to check the base path because
  	// we are guaranteed that both paths are either relative or absolute.
  	absolute = cfs_path_is_root_absolute(style, path_base, base_root_length);

  	// We must keep track of the end of the previous segment. Initially, this is
  	// set to the beginning of the path. This means that 0 is returned if the
 	// first segment is not equal.
  	end = path_base + base_root_length;

  	// Now we loop over both segments until one of them reaches the end or their
  	// contents are not equal.
  	do {
    	// We skip all segments which will be removed in each path, since we want to
    	// know about the true path.
    	if (!cfs_path_segment_joined_skip_invisible(style, &base, absolute) ||
        	!cfs_path_segment_joined_skip_invisible(style, &other, absolute)) {
      		break;
    	}

    	if (!cfs_path_string_equal(style, base.segment.begin, other.segment.begin,
          	base.segment.size, other.segment.size)) {
      		// So the content of those two segments are not equal. We will return the
      		// size up to the beginning.
      		return (size_t)(end - path_base);
    	}

    	// Remember the end of the previous segment before we go to the next one.
    	end = base.segment.end;
  	} while (cfs_path_get_next_segment_joined(style, &base) &&
			 cfs_path_get_next_segment_joined(style, &other));

  	// Now we calculate the length up to the last point where our paths pointed to
  	// the same place.
  	return (size_t)(end - path_base);
}

size_t cfs_path_get_intersection(const char *path_base, const char *path_other) {
	return cfs_path_get_intersection_impl(CFS_PATH_UNIX, path_base, path_other);
}

size_t cfs_plat_path_get_intersection(const char *path_base, const char *path_other) {
	return cfs_path_get_intersection_impl(path_style, path_base, path_other);
}

static size_t cfs_path_output_sized(char* buffer, size_t buffer_size, size_t position, const char* str, size_t length) {
  size_t amount_written;

  // First we determine the amount which we can write to the buffer. There are
  // three cases. In the first case we have enough to store the whole string in
  // it. In the second one we can only store a part of it, and in the third we
  // have no space left.
  if (buffer_size > position + length) {
    amount_written = length;
  } else if (buffer_size > position) {
    amount_written = buffer_size - position;
  } else {
    amount_written = 0;
  }

  // If we actually want to write out something we will do that here. We will
  // always append a '\0', this way we are guaranteed to have a valid string at
  // all times.
  if (amount_written > 0) {
    memmove(&buffer[position], str, amount_written);
  }

  // Return the theoretical length which would have been written when everything
  // would have fit in the buffer.
  return length;
}

static void cfs_path_terminate_output(char* buffer, size_t buffer_size, size_t position) {
	if(buffer_size > 0) {
		if(position >= buffer_size) {
			buffer[buffer_size - 1] = '\0'; 
		} else {
			buffer[position] = '\0';
		}
	}
}

static size_t cfs_path_output_separator(cfs_path_style style, char* buffer, size_t buffer_size, size_t position) {
	return cfs_path_output_sized(buffer, buffer_size, position, seperators[style], 1);
}

static size_t cfs_path_output_current(char* buffer, size_t buffer_size, size_t position) {
	return cfs_path_output_sized(buffer, buffer_size, position, ".", 1);
}

static size_t cfs_path_join_and_normalize_multiple(cfs_path_style style, const char** paths, char* buffer, size_t buffer_size) {
	size_t pos;
	bool absolute, has_segment_output;
	cfs_segment_joined sj;

	cfs_path_get_root(style, paths[0], &pos);
	
	absolute = cfs_path_is_root_absolute(style, paths[0], pos);

	cfs_path_output_sized(buffer, buffer_size, 0, paths[0], pos);

	if(!cfs_path_get_first_segment_joined(style, paths, &sj)) {
		goto done;
	}

	has_segment_output = false;

	do {
		if(cfs_path_segment_will_be_removed(style, &sj, absolute)) {
			continue;
		}

		if(has_segment_output) {
			pos += cfs_path_output_separator(style, buffer, buffer_size, pos);
		}

		has_segment_output = true;
		
		pos += cfs_path_output_sized(buffer, buffer_size, pos, sj.segment.begin, sj.segment.size);
	} while(cfs_path_get_next_segment_joined(style, &sj));

	if(!has_segment_output && pos == 0) {
		assert(absolute == false);
		pos += cfs_path_output_current(buffer, buffer_size, pos);
	}

done:
	cfs_path_terminate_output(buffer, buffer_size, pos);

	return pos;
}

static size_t cfs_path_normalize_impl(cfs_path_style style, const char* path, char* buffer, size_t buffer_size) {
	const char* paths[2];

	paths[0] = path;
	paths[1] = NULL;

	return cfs_path_join_and_normalize_multiple(style, paths, buffer, buffer_size);
}


static size_t cfs_path_get_absolute_impl(cfs_path_style style, const char* base, const char* path, char* buffer, size_t buffer_size) {
	size_t i;
	const char* paths[4];

	if(cfs_path_is_absolute(base)) {
		i = 0;
	} else if(style == CFS_PATH_WINDOWS) {
		paths[0] = "\\";
		i = 1;
	} else {
		paths[0] = "/";
		i = 1;
	}

  	if (cfs_path_is_absolute(path)) {
    // If the submitted path is not relative the base path becomes irrelevant.
    // We will only normalize the submitted path instead.
    paths[i++] = path;
    paths[i] = NULL;
  } else {
    // Otherwise we append the relative path to the base path and normalize it.
    // The result will be a new absolute path.
    paths[i++] = base;
    paths[i++] = path;
    paths[i] = NULL;
  }

  // Finally join everything together and normalize it.
  return cfs_path_join_and_normalize_multiple(style, paths, buffer, buffer_size);

}
