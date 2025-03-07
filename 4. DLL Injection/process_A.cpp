#include <Windows.h>
#include <stdio.h>
#include <tlhelp32.h>

#define P_ERROR(msg) \
    do { \
        fprintf(stderr, "[!]\t%s. Error: %lu\n", (msg), GetLastError()); \
    } while (0)

BOOL GetProcessHandle(const wchar_t* lpProcessName, HANDLE* hpProcess) {
	HANDLE hSnapshot;
	if ((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL)) == INVALID_HANDLE_VALUE) {
		P_ERROR("Could not load snapshot");
		return FALSE;
	}

	PROCESSENTRY32W pe32 = {
		.dwSize = sizeof(PROCESSENTRY32)
	};

	if (!Process32First(hSnapshot, &pe32)) {
		P_ERROR("Could not get first process from snapshot");
		return FALSE;
	}

	do
	{
		if (wcscmp(lpProcessName, pe32.szExeFile) == 0) {
			printf("[+]\tFound process, PID: %d\n", pe32.th32ProcessID);
			if ((*hpProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID)) == NULL) {
				P_ERROR("Could not open process");
				return FALSE;
			}
			CloseHandle(hSnapshot);
			return TRUE;
		}

	} while (Process32Next(hSnapshot, &pe32));

	printf("[!]\tCould not find process name");
	return FALSE;
}

BOOL InjectDLL(HANDLE hProcess, const wchar_t* lpDLLName) {
	FARPROC fnLoadLibraryW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
	if (fnLoadLibraryW == NULL) {
		P_ERROR("Could not get process address of LoadLibraryW");
		return FALSE;
	}
	
	LPVOID pBuffer;
	if ((pBuffer = VirtualAllocEx(hProcess, NULL, wcslen(lpDLLName) * sizeof(wchar_t), (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE)) == NULL) {
		P_ERROR("Could not allocate virtual memory to process");
		return FALSE;
	}

	printf("Buffer address: 0x%p\n", pBuffer);

	SIZE_T nob;

	if (!WriteProcessMemory(hProcess, pBuffer, lpDLLName, wcslen(lpDLLName) * sizeof(wchar_t), &nob)) {
		P_ERROR("Could not write to process memory");
		return FALSE;
	}

	printf("Written:%zu, length: %zu\n", nob, wcslen(lpDLLName));

	HANDLE hThread;
	DWORD TID;
	if ((hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) fnLoadLibraryW, pBuffer, 0, &TID)) == NULL) {
		P_ERROR("Could not create remote thread");
		return FALSE;
	}

	printf("TID: %d", TID);

	WaitForSingleObject(hThread, INFINITE);
	return TRUE;
}

int main() {
	HANDLE hProcess;
	if (!GetProcessHandle(L"smallprocess.exe", &hProcess)) {
		return -1;
	}
	return InjectDLL(hProcess, L"C:\\Users\\xoort\\Desktop\\Projects\\Crow stuff\\InjectDLL\\x64\\Release\\InjectDLL.dll");
}

