#include "Memory.hpp"
#include "../utils/NTDLL_FAST.hpp"
#include <iostream>
#include <string>
#include <sstream>

#define PAGE_READABLE (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE)

HMODULE NTDLL = NULL;
#define MF(Name,Module) ((ULONG_PTR (*)(...))GetProcAddress(Module,Name))
#define NtF(Name) MF(Name,NTDLL)

Memory* Memory::g_Singleton = nullptr;

Memory* Memory::get_singleton() noexcept {
    if (g_Singleton == nullptr)
        g_Singleton = new Memory();
    return g_Singleton;
}

DWORD Memory::get_process_id(LPCTSTR process_name)
{
    PROCESSENTRY32 pt{};
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pt.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hsnap, &pt))
    {
        do {
            if (!lstrcmpi(pt.szExeFile, process_name))
            {
                CloseHandle(hsnap);
                return pt.th32ProcessID;
            }
        } while (Process32Next(hsnap, &pt));
    }
    CloseHandle(hsnap);
    return 0;
}

void Memory::init_NTfunctions() {
    NTDLL = GetModuleHandle("ntdll.dll");

    if (NTDLL == NULL)
        return;

    NTDLL_INIT_FCNS(NTDLL);
}

bool Memory::setup(DWORD proc_id)
{
    this->process_id = proc_id;

    if (!process_id)
        return false;

    this->RobloxProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, this->process_id);

    if (!this->RobloxProcess)
        return false;

    return true;
}

DWORD Memory::get_main_thread_id() {
    if (this->mainThreadID != NULL) {
        return this->mainThreadID;
    }

    SYSTEM_PROCESS_INFORMATION* Processes = NULL;
    SYSTEM_PROCESS_INFORMATION* ProcessCur;
    //===========================================
    Processes = (SYSTEM_PROCESS_INFORMATION*)(new char[10000000]);
    while (1) {

        while ((DWORD)NtF("NtQuerySystemInformation")(SystemProcessInformation, Processes, 10000000, NULL) != 0) {};
        //============================================
        DWORD proc = 0;
        ProcessCur = Processes;
        while (ProcessCur->NextOffset) {
            if (ProcessCur->ImageName.Buffer != 0) {
                if (wcscmp(ProcessCur->ImageName.Buffer, L"RobloxPlayerBeta.exe" ) == 0) {
                    this->mainThreadID = ProcessCur->ThreadInfos[0].Client_Id.UniqueThread;
                    return this->mainThreadID;
                }
            }
            ProcessCur = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)ProcessCur + ProcessCur->NextOffset);
        }
    }

    if (!ProcessCur)
        return NULL;
}

void Memory::writem(LPVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T  nSize, SIZE_T* lpNumberOfBytesRead)
{
    if (!this->RobloxProcess)
        return;

    MEMORY_BASIC_INFORMATION bi;
    VirtualQueryEx(this->RobloxProcess, lpBaseAddress, &bi, sizeof(MEMORY_BASIC_INFORMATION));


    BOOL r = WriteProcessMemory(this->RobloxProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);


    PVOID baddr = bi.AllocationBase;
    SIZE_T size = bi.RegionSize;
    NtUnlockVirtualMemory(this->RobloxProcess, &baddr, &size, 1);
}
LPVOID Memory::allocate(int size)
{
    if (!this->RobloxProcess)
        return 0;

    return VirtualAllocEx(this->RobloxProcess, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void Memory::readm(LPCVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T  nSize, SIZE_T* lpNumberOfBytesRead)
{
    if (!this->RobloxProcess)
        return;


    MEMORY_BASIC_INFORMATION bi;
    VirtualQueryEx(this->RobloxProcess, lpBaseAddress, &bi, sizeof(MEMORY_BASIC_INFORMATION));


    BOOL r = ReadProcessMemory(this->RobloxProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);

    PVOID baddr = bi.AllocationBase;
    SIZE_T size = bi.RegionSize;
    NtUnlockVirtualMemory(this->RobloxProcess, &baddr, &size, 1);
}

bool Memory::suspendProcess() {
    if (!this->process_id) return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == this->process_id) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    NtSuspendThread(hThread, nullptr);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te32));
    }
    CloseHandle(hSnapshot);
    return true;
}

bool Memory::resumeProcess() {
    if (!this->process_id) return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == this->process_id) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    NtResumeThread(hThread, nullptr);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te32));
    }
    CloseHandle(hSnapshot);
    return true;
}

bool Memory::suspend5Threads() {
    if (!this->process_id) return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    int threadCount = 0;
    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == this->process_id) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    NtSuspendThread(hThread, nullptr);
                    CloseHandle(hThread);
                    threadCount++;
                }
                if (threadCount >= 5) break;
            }
        } while (Thread32Next(hSnapshot, &te32));
    }
    CloseHandle(hSnapshot);
    return true;
}

bool Memory::resume5Threads() {
    if (!this->process_id) return false;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    int threadCount = 0;
    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == this->process_id) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    NtResumeThread(hThread, nullptr);
                    CloseHandle(hThread);
                    threadCount++;
                }
                if (threadCount >= 5) break;
            }
        } while (Thread32Next(hSnapshot, &te32));
    }
    CloseHandle(hSnapshot);
    return true;
}