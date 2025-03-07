## Introduction
Before we can get our hands dirty with process injection, we first need to know what a process is and how they work.

>Later in this post we'll be using [x64dbg]([https://x64dbg.com/](https://x64dbg.com/)) to view the internals of an ongoing process, so make sure you've got it installed (or any other debugging tool).

## Abstract definition

If you're reading this post and you're interested in malware development, chances are you do have a basic idea of what a process is. To cover all our bases, though, I will provide a definition:

*A process is a collection of machine instructions that live in computer memory.* 

Those machine instructions can then be carried out by the CPU to perform some sort of function for the user. Apart from just the executable machine code instructions, processes can and usually do contain other data as well, such as file handles, connection handles or variables.

A **process** is different from a **program**, although they are very much related. When we talk about a program, we're usually talking about an **image** of the executable machine code that lives in storage. On Windows, such an image is usually (but not exclusively) in the form of a `.exe` file. When we execute such an image, those instructions are transferred into memory, execution is started and thus a process is born.

Even more so, an executable image is different from **source code**. If you're like me and you come from a software development background, you might instinctively think of the lines in your source code as the instructions being executed by your computer. While this thought process works great for developing software, we need to get rid of this idea if we're analysing malware. The whole point of process injection is that we'll get a process to execute instructions that were *nowhere to be found* in the source code.

## Under the loop

We now know that a process is a region of memory on a computer that contains machine instructions and data. The image below explains (in a somewhat simplified way) how this memory is partitioned.

<img width="457" alt="image" src="https://github.com/user-attachments/assets/443c5e1b-3368-47fe-b1d0-f0db31377701" />

The bottom section called **code** contains the machine code instructions that the CPU has to execute. These were stored in the program image. The "data" and "stack" sections contain the aforementioned data like variables.

>This is a simplified explanation of the memory layout of a process, the real thing is a lot more complex.

Let's take a look at an actual ongoing process on our machine, to see what's happening in practice. To this end, I'll create a very simple program in `C`, although you could just pick any process you can find in your process explorer.

```
// ProcessExplainer.c
#include <iostream>

int main()
{
    std::cout << "Hello World!\n";
    return 0;
}
```

After compiling this program we'll open it up in `x64dbg`. We're met with the following:

<img width="770" alt="Pasted image 20250305125359" src="https://github.com/user-attachments/assets/503abed9-7dbe-4dfe-b8dc-64c28e2ea3d1" />


If you've never looked at the inside of a process before, the amount of information on your screen can be overwhelming. `x64dbg` is an incredibly complex program and by no means should you know what's going on in every part of it. For now, we're only trying to get an _idea_ of what a process looks like under the hood. 

Pay attention to the largest, upper-left window. Each of the lines you see there, is a machine code instruction. The CPU executes these instructions line by line (unless it's explicitly told to start executing from a different line). The instructions themselves are quite simple in nature, go ahead and scroll through the instructions, see if you can infer the functions of some of them. Don't worry about it though, at this stage it's not necessary at all to be aware of what these instructions do. You'll likely notice that even though our source code was about as simple as it gets, all of the instructions in the process certainly aren't. 

`x64dbg` is kind enough to automatically set a breakpoint at the start of the main function. This means that the highlighted line you see, is the instruction that the CPU is about to execute right now. It also means that we're currently looking at the part in memory that stores the instructions of this process. As I stated before though, a process consists of more than just instructions. Head over to the "Symbols" tab and click on the `processexplainer.exe` (or whatever your `.exe` file is called). A new window to the right should open up. In the search bar below it, type "Hello world". You should see our "Hello world" string saved in memory. Right click this line and click on "follow in Dump" to see that actual bytes of this variable stored between a myriad of other information.

## Injection

Now that we have a better understanding of the internals of a process, let's consider what it means to *inject* into a process:

1. We create new region of space in the memory that is the target process
2. We write (malicious) machine instructions to this region
3. We have the CPU execute those instructions

In short, we add a few lines of instructions to the already giant list of instructions you can see with `x64dbg`, point the CPU to the first one.
