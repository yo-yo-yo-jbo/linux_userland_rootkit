# Coding a Linux userland rootkit

This blopost could be a nice opportunity to talk about some basic Linux concepts, including `procfs`, `LD_PRELOAD`, hooking etc..  
While I will not do justice to the word "rootkit" in terms of sophistication, we will be creating a rootkit, in fact.

## Motivation
According to [Wikipedia](https://en.wikipedia.org/wiki/Rootkit):
> A rootkit is a collection of computer software, typically malicious, designed to enable access to a computer or an area of its software that is not otherwise allowed (for example, to an unauthorized user) and often masks its existence or the existence of other software.

We will be focusing on the "masks its existence" part. Our goal for today would be creating a piece of code that can hide an arbitrary filename and process substring.  
We will be assuming we have privileged aribtrary code execution as the `root` user.
