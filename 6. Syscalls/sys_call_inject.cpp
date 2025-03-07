// NTAPI Injection.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Header.h"


int main(int argc, char* argv[]) {

    // Variables

    DWORD             PID = NULL;
    HANDLE            hProcess = NULL;
    HANDLE            hThread = NULL;
    HMODULE           hKernel32 = NULL;
    HMODULE           hNTDLL = NULL;
    PVOID             rBuffer = NULL;

    wchar_t           dllPath[MAX_PATH] = L"C:\\Users\\xoort\\Desktop\\Projects\\Crow stuff\\InjectDLL\\x64\\Debug\\InjectDLL.dll";
    SIZE_T            pathSize = sizeof(dllPath);
    size_t            bytesWritten = 0;

    OBJECT_ATTRIBUTES OA = { sizeof(OA), NULL };

    // Check correct usage

    if (argc < 2) {
        warn("usage: %s <PID>", argv[0]);
        return EXIT_FAILURE;
    }
    PID = atoi(argv[1]);

    // Handle to Kernel32.dll and NTDLL.dll modules

    info("Trying to get handles to Kernel32 and NTDLL");

    hKernel32 = GetModuleHandleW(L"kernel32.dll");
    hNTDLL = GetModuleHandleW(L"ntdll.dll");

    if (hKernel32 == NULL) {
        warn("failed to get a handle to Kernel32. error: 0x%lx\n", GetLastError());
        return EXIT_FAILURE;
    }
    if (hNTDLL == NULL) {
        warn("failed to get a handle to NTDLL. error: 0x%lx\n", GetLastError());
        return EXIT_FAILURE;
    }


    // Opening handle to process
    info("trying to get a handle to the process (%ld)", PID);
    NTSTATUS status;
    CLIENT_ID ci = { (HANDLE)PID, NULL };
    status = NtOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &OA, &ci);
    if (status != STATUS_SUCCESS) {
        warn("Could not open process, error: 0x%lx", status);
        return EXIT_FAILURE;
    }

    // Reserving and commiting memory in target process
    info("Reserving and commiting memory in target process");
    status = NtAllocateVirtualMemory(hProcess, &rBuffer, NULL, &pathSize, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
    if (status != STATUS_SUCCESS) {
        warn("Could not reserve/commit virtual memory, error: 0x%lx", status);
        return EXIT_FAILURE;
    }

    // Write to allocated memory
    info("Writing to allocated memory");
    status = NtWriteVirtualMemory(hProcess, rBuffer, dllPath, sizeof(dllPath), &bytesWritten);
    if (status != STATUS_SUCCESS) {
        warn("Could not write to virtual process of memory, error: 0x%lx", status);
        return EXIT_FAILURE;
    }

    // Starting remote thread
    PTHREAD_START_ROUTINE xLoadLibrary = (PTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    info("Starting remote thread");
    status = NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS, &OA, hProcess, xLoadLibrary, rBuffer, FALSE, NULL, NULL, NULL, NULL);

    if (status != STATUS_SUCCESS) {
        warn("failed to create thread, error: 0x%lx", status);
        return EXIT_FAILURE;
    }

    info("Waiting for remote thread to finish execution");
    WaitForSingleObject(hThread, INFINITE);
    okay("Thread finished");

    // Cleanup

    info("Cleaning up");
    if (hProcess) {
        CloseHandle(hProcess);
    }
    if (hThread) {
        CloseHandle(hThread);
    }

    okay("All done, goodbye!");
    return EXIT_SUCCESS;
}