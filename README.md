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
