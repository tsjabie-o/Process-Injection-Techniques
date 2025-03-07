# DLL Injection
## Introduction
In this post, we'll take a look at another method of process injection, known as **DLL-injection**. 

In our previous **Shellcode Injection** method, *process A* directly injected (malicious) code into the instruction region in memory of *process B*. This time, the malicious code will be hosted in a separate `.dll` file (we'll go over what a `.dll` file is in a second), which *process A* will force *process B* to load. 

We'll list some possible advantages of this approach:
- **Modularity and ease of implementation**: For a simple one-off function, shellcode works fine. However if the injected code is complex enough, with lots of logic and possible execution paths, it's much more convenient to be able to program and compile a `.dll` file. 
- **Persistence**: A DLL can safely stay loaded in a process as long as it lives, while shellcode is only executed once (and possibly causes the host process to crash out).
- **Evasion**: Since a DLL can blend in on a target system better, and our *process A* now does not contain shellcode that is very recognisable as suspicious at the least, this method is somewhat stealthier.

>It should be said though that this is by no means a stealthy approach anymore in this day and age. The naive approach that we're about to code will most certainly be recognised and prevented by any modern AV.

>To fully understand the techniques described in this post, some knowledge of the concept of **virtual memory** is required.

## What is a `.dll` file?
`.dll` (Dynamic Link Library) files are another form of the so called **Portable Executable (PE)** files inside the Windows ecosystem, just like the well-known `.exe` files. Both these types of files can contain instructions and data. However, unlike `.exe` files, `.dll` files are not meant to be used as standalone executables. Instead, they're meant to be dynamically loaded by other processes, when needed, so that process can then access functions inside of them. `.dll` files contain more general functions that many different processes might use, such as file I/O or networking functions. As such, the existence of DLL files means that processes don't need to create and define their own versions of these functions, saving lots of storage, memory and time spent by programmers. Thus they allow for more **code reusability** and **modularity**.

>Loaded DLL files are often referred to as **modules**

Open up any process running on a system, check its loaded modules and you'll see it probably has a multitude `.dll` files loaded. Cross check against other processes and you'll see many of them have the same ones loaded as well, immediately showing their usefulness.

For example, here you can see that both the calculator and the notepad process both load many of the same `.dll` files, like the `kernel32.dll`.

![Pasted image 20250306175134](https://github.com/user-attachments/assets/9890c94b-34d2-4042-9f3c-e924fc273ca4)

![Pasted image 20250306175302](https://github.com/user-attachments/assets/0bedfd31-2acd-4070-b3e2-d55aca2ce716)


## Creating a `.dll` file
Let's code and build or very own DLL. The very easiest way is to use Visual Studio and when creating a new project, choose the DLL template. You could also do this manually, setting up your build chain to handle it the right way, but I won't do that here. Though it's not strictly necessary, we'll write our DLL in C/C++, which is also what Visual Studio assumes and supports.

A blank DLL source file will look something like this:

```C
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD ul_reason_for_call,
	LPVOID lpReserved){
		switch (ul_reason_for_call){
			case DLL_PROCESS_ATTACH:
			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
	}
	return TRUE;
}
```

What we see here are basically the four possible points of entry into our DLL. From top to bottom, the respective case is executed when the DLL
1. is loaded by another process
2. is loaded by a new thread in a process
3. is unloaded by a single thread in a process
4. is unloaded by another process

Making sure the right case is selected is handled by the process that's loading the DLL and/or the OS. 

The case we're mostly interested in is the first one. This is were we can write some malicious code that will then execute when *process A* forces *process B* to load our `.dll` file.

With that in mind, there's really no difference from regular C/C++ coding. For our injection purposes, we'll just have the DLL call `MessageBoxW()`, which just causes a dialog box to pop up.

```C
case DLL_PROCESS_ATTACH:
	MessageBoxW(NULL, L"Hello, World!", L"XXX", MB_ICONASTERISK);
	break;
```

## Preparing the injection
Now that we've got a "malicious" `.dll` file, let's work on injecting it. This actually works very similarly to how we performed shellcode injection: we create a *process A* and use the WIN32 API calls to create memory and write instructions in process B, and tell it to execute those instructions. Indeed, we're just injecting some code again. This time though, the instructions we inject will only tell *process B* to load our DLL. The DLL will contain the actual malicious code we want the process to execute. Again, refer to the introduction section to learn why one would choose this method over just directly injecting the malicious instructions.

But exactly which instructions are we injecting into *process B* in order to get it to load our DLL? We tell it to execute the `LoadLibraryW()` function ([documentation](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya)) with the name of our DLL as an argument. This function simply loads a `.dll` file into memory, which is exactly what we need. There's two things we need to take care of here, **(1) *process B* needs to execute this function** and **(2) it needs to supply our DLL as an argument to that function**.

### (1)
Not as easy as it may sound, but if you understood the previous post about shellcode injection this method should be easy to understand. Remember that we used the following code for *process B* to execute our shellcode:

```C
hThread = CreateRemoteThreadEx(
	hProcess,
	NULL,
	0,
	(LPTHREAD_START_ROUTINE)rBuffer,
	NULL,
	0,
	0,
	&dwTID
);
```

With the `(LPTHREAD_START_ROUTINE)rBuffer` argument, we pointed it to the address of the start of the instructions - our shellcode in this case - to execute. We injected those instructions before. This time, we need to provide the address of the `LoadLibraryA` function, which lives inside the `kernel32.dll` file. We can find it like this:

```C
FARPROC fnLoadLibraryW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW")
```

Let's unpack that. First off all `FARPROC` is just a generic function pointer type in the WIN32 API ecosystem (look at Windows' documentation if you'd like to know more). `GetModuleHandleW()` finds the handle of a loaded module, such as a `.dll` file. Since `kernel32.dll` is a system DLL with core functions, it's loaded by every process, so we know *process A* will have it loaded too. Next `GetProcAddress()` takes the handle of a module and finds the address in memory of a specific function in that module, referred to by a string of its name. So now, `fnLoadLibraryW` contains some address (like `0x000345A5D`) which in memory is the start of the `LoadLibrayW()` function, in the region of the loaded `kernel23.dll`.

You might be wondering: Just because our *process A* has `kernel32.dll` and `LoadLibraryW()` loaded at that specific address, why should *process B* have it loaded exactly there too? Good question! Modern Windows does use **Address Space Layout Randomization (ASLR)** when loading `.dll` files - to prevent exploits like these. This means DLLs are loaded onto random base addresses across different processes, making it impossible to perform the technique we describe here. However, since system DLLs like `kernel32.dll` are loaded by every process, ASLR is not applied to them. This allows all processes to share the same physical memory pages containing these system DLLs, saving a lot of memory overhead.

![Pasted image 20250307100931](https://github.com/user-attachments/assets/3195ef32-5393-44ec-9f82-33e9c2195362)

![Pasted image 20250307100953](https://github.com/user-attachments/assets/bfe77b1c-0bc3-41a6-9392-fe000634f62d)


>The addresses of these system DLLs is randomised at boot, but consistent across processes

Cool, we know have the address of `LoadLibraryW()` ready to be passed to *process B*. Let's move on to the next step.

### (2)
Now that we can get *process B* to execute the `LoadLibraryW()` function, we need to have it use our DLL filename as an argument. With `CreateRemoteThread()`, we should pass this argument to-be-used as an address as well. This means the string containing our DLL filename should already be present in the memory of *process B*, so we can supply its address. How do we get it there? The same way that we got our shellcode into its memory in the previous post; using `VirtualAllocEx()` and `WriteProcessMemory`.

```C
// Get a handle to the process
hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
if (hProcess == NULL) {
	return EXIT_FAILURE;
}

// Create our DLL filename and path as a string and compute its size
wchar_t* lpDLLName = L"Path/To/Our/Malicious.dll"
size_t sDLLNameSize = sizeof(lpDLLName);

// Allocate memory in process B that can hold our DLL filename
rBuffer = VirtualAllocEx(
	hProcess,
	NULL,
	payloadSize,
	(MEM_COMMIT | MEM_RESERVE),
	PAGE_EXECUTE_READWRITE
);

// Write that filename to the allocated memory
WriteProcessMemory(
	hProcess,
	rBuffer,
	payload,
	payloadSize,
	NULL
)
```

Let's check if this all went according to plan. In my case, the filename was `C:\\Users\\xoort\\Desktop\\Projects\\Crow stuff\\InjectDLL\\x64\\Release\\InjectDLL.dll`

Address in the memory space of the calculator app that the DLL filename was written to:

![Pasted image 20250307101537](https://github.com/user-attachments/assets/13452074-1b42-4038-8ec2-7a7d8d7f3a88)


That address inspected with `x64dbg`:

![Pasted image 20250307101655](https://github.com/user-attachments/assets/01a20824-68c9-4f71-826f-0db0ae37dd2f)



## Performing the injection
With everything set up, we can combine what we've done so far and perform the injection.

Remember, we have the address of `LoadLibraryW()` saved in `fnLoadLibraryW` and the string with our DLL filename is already present in the memory of *process B* at addres `rBuffer`.

```C
hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) fnLoadLibraryW, pBuffer, 0, &TID)
```

That's it! Now *process B* will have loaded your DLL and as such the dialog box that we told it to code should pop up.

To verify, you can also check out all of the threads in your target process and see your newly created one. To do that, pass `CREATE_SUSPENDED` to `CreateRemoteThread()` so the thread is suspended and does not immediately execute and disappear.

![Pasted image 20250307105146](https://github.com/user-attachments/assets/4611e61f-ff70-4bb3-9f29-2047cf717dae)
