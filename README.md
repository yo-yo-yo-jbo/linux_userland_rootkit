# Coding a Linux userland rootkit

This blopost could be a nice opportunity to talk about some basic Linux concepts, including `procfs`, `LD_PRELOAD`, hooking etc..  
While I will not do justice to the word "rootkit" in terms of sophistication, we will be creating a rootkit, in fact.

## Motivation
According to [Wikipedia](https://en.wikipedia.org/wiki/Rootkit):
> A rootkit is a collection of computer software, typically malicious, designed to enable access to a computer or an area of its software that is not otherwise allowed (for example, to an unauthorized user) and often masks its existence or the existence of other software.

We will be focusing on the "masks its existence" part. Our goal for today would be creating a piece of code that can hide an arbitrary filename and process substring.  
We will be assuming we have privileged aribtrary code execution as the `root` user.

## Shared object preload
Our strategy will be simple - we will *inject* code into all new processes and *hook* relevant functions to hide our files and processes.  
I talked a bit about injection and hooking [in the past](https://github.com/yo-yo-yo-jbo/injection_and_hooking_intro/) - it was on Windows, but the concepts are similar.  
Instead of `DLLs`, we will use `.so` files. Those files are `shared objects` and have the same file format as Linux executables (ELFs). They export symbols that can be used later, *conceptually* like Windows does.  Just like Windows has `LoadLibrary` and `GetProcAddress`, Linux has `dlopen` and `dlsym`, and just like Windows PE files can have dependencies on DLLs, Linux ELFs have dependencies on shared objects.  
Anyway, our strategy will be the following:
- For injection, we will use the `/etc/ld.so.preload` file. To write to it, we will need root access, but essentially - all files mentioned in that file (that might not exist to begin with) are going to be loaded to new processes. If you're familiar with the `LD_PRELOAD` environment variable, it's the same concept, but done persistently.
- We get hooking for free. Unlike the Windows dynamic library resolution, that import symbols from specific DLLs (e.g. `kernel32!MapViewOfFile`), on Linux the loader just needs a symbol - once the symbol is resolved it doesn't matter what shared object it comes from. The only thing we need to do is export the right symbol.

## Hiding files
Let's start with hiding files. We will be hooking the [readdir](https://man7.org/linux/man-pages/man3/readdir.3.html) function, which returns a `struct dirent*` - that's a *directory entry* that contain information about the file, including its name:

```c
struct dirent {
   ino_t          d_ino;       /* Inode number */
   off_t          d_off;       /* Not an offset; see below */
   unsigned short d_reclen;    /* Length of this record */
   unsigned char  d_type;      /* Type of file; not supported
                                  by all filesystem types */
   char           d_name[256]; /* Null-terminated filename */
};
```

The `readdir` function returns `NULL` on error or if the directory list is done, but we can simply call the next entry. For that, however, we'll need to call the original `readdir` function, which can be easily done with `dlopen` and `dlsym`:

```c
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>

#define FILENAME_TO_HIDE ("example")

typedef struct dirent* (*readdir_pfn_t)(DIR*);

static
readdir_pfn_t
g_original_readdir = NULL;

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
```

Let's examine this code carefully:
- We include several headers - `dlfcn.h` for `dlsym` related stuff, `dirent.h` for the `struct dirent` definition, and `string.h` for `strstr`.
- Since this is only an exercise `FILENAME_TO_HIDE` is simply defined as a constant. In a real offensive tool we'd probably not just have a constant, since we do not want to recompile every time we want to hide a file. Also, we'd probably support multiple filenames.
- We define a `readdir_pfn_t` type, which is the function pointer definition for `readdir`, and declare one global variable `g_original_readdir` which will point to the original `readdir` function. We will initialize it lazily (i.e. the first time it's needed) but we could've also initialized it in the `__attribute(constructor)__` function (we we'd have to declare) - that's the shared object equivalent of `DllMain`.
- We create a new function `readdir` that hooks the original function. As we said, it first resolves the original `readdir` by using the `dlsym` function, which can get a `RTLD_NEXT` constant to get the *next symbol* - in our case, that'd be the original one.
- We then create a loop that calls the original `readdir` function - if it fails or finishes (indicated by returning `NULL`) then we simply return the `NULL` result. Otherwise, we check that the `FILENAME_TO_HIDE` constant is not in the `d_name` member in the directory entry (by using `strstr`). This means we skip filenames we wish to hide.

