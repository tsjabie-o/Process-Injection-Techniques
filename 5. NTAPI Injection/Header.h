#pragma once
#pragma comment (lib, "ntdll")

#include <stdio.h>
#include <Windows.h>

#define okay(msg, ...) printf("[+] " msg "\n", ##__VA_ARGS__)
#define warn(msg, ...) printf("[!!] " msg "\n", ##__VA_ARGS__)
#define info(msg, ...) printf("[i] " msg "\n", ##__VA_ARGS__)

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG Length;                                                           //0x0
    VOID* RootDirectory;                                                    //0x8
    struct _UNICODE_STRING* ObjectName;                                     //0x10
    ULONG Attributes;                                                       //0x18
    VOID* SecurityDescriptor;                                               //0x20
    VOID* SecurityQualityOfService;                                         //0x28
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef struct _PS_ATTRIBUTE
{
    ULONG Attribute;
    SIZE_T Size;
    union
    {
        ULONG Value;
        PVOID ValuePtr;
    };
    PSIZE_T ReturnLength;
} PS_ATTRIBUTE, * PPS_ATTRIBUTE;

typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, * PPS_ATTRIBUTE_LIST;

typedef struct _CLIENT_ID
 {
     HANDLE UniqueProcess;
     HANDLE UniqueThread;
 } CLIENT_ID, * PCLIENT_ID;

typedef NTSTATUS(NTAPI *pNtCreateThreadEx) (
    _Out_    PHANDLE            ThreadHandle,
    _In_     ACCESS_MASK        DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_     HANDLE             ProcessHandle,
    _In_     PVOID              StartRoutine,
    _In_opt_ PVOID              Argument,
    _In_     ULONG              CreateFlags,
    _In_     SIZE_T             ZeroBits,
    _In_     SIZE_T             StackSize,
    _In_     SIZE_T             MaximumStackSize,
    _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
);

typedef NTSTATUS(NTAPI *pNtOpenProcess) (
     _Out_ PHANDLE ProcessHandle,
     _In_ ACCESS_MASK DesiredAccess,
     _In_ POBJECT_ATTRIBUTES ObjectAttributes,
     _In_opt_ PCLIENT_ID ClientId
);

typedef NTSTATUS(NTAPI *pNtAllocateVirtualMemory) (
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*RegionSize) _Writable_bytes_(*RegionSize) _Post_readable_byte_size_(*RegionSize)) PVOID* BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect
);

typedef NTSTATUS(NTAPI *pNtWriteVirtualMemory) (
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
);

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)