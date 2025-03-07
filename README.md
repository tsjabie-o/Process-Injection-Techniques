In this repository, we will be exploring the workings of **process injection** and create our own process injecting executable. We'll investigate a few different methods of performing process injection:

- *Shellcode Injection*
- *DLL Injection*
- *NTAPI Injection*
- *(In)direct System Calls Injection*

Along the way, we'll be diving into the inner workings of the Windows operating system, such as the *Win32 API* and DLLs, as well as getting our hands dirty with debuggers like `x32dbg`. 

>[!warning] The code I provide here is not written with portability in mind and contains quite some hardcoded values specific to my machine. So don't expect to be able to clone and run it immediately. It serves more as reference material

>[!info] This entire project is based upon a few tutorials made by [crow](https://www.youtube.com/@crr0ww). I am writing these posts because I want to further reinforce what I learn myself and explaining concepts to others is a [great way to do this](https://aliabdaal.com/studying/the-feynman-technique/). 

## Roadmap
Before we can dive into the actual injection, we need to go over some basics. This will give us an understanding of the inner workings of the Windows operating system (or at least, the part that we're interested in for now) and allow us to understand how our injection actually works under the hood.

1. Processes in Windows
2. The WIN32 API
3. Shellcode Injection
4. DLL Injection
5. NTAPI Injection*
6. System Calls*

>[!info] *Posts about NTAPI Injection and System Calls are still a work in progress. The code is already available though.

## Knowledge prerequisites
While these posts are certainly aimed at beginners and I will be going over most of the core concepts, you should generally already have some _basic_ knowledge about computers, operating systems and programming. Most importantly, you should be somewhat familiar with programming in C/C++ and with the concept of virtual memory.