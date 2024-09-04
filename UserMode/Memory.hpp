#pragma once


#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <iostream>


class Memory {
    static Memory* g_Singleton;
public:
    static Memory* get_singleton() noexcept;

    int process_id;
    DWORD mainThreadID;
    HANDLE RobloxProcess;

    DWORD get_process_id(LPCTSTR process_name);

    bool setup(DWORD proc_id);

    DWORD get_main_thread_id();

    void init_NTfunctions();

    void writem(LPVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T  nSize, SIZE_T* lpNumberOfBytesRead);

    void readm(LPCVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T  nSize, SIZE_T* lpNumberOfBytesRead);

    LPVOID allocate(int size);

    template<typename T> void write(uintptr_t address, T value)
    {
        writem((LPVOID)address, &value, sizeof(T), NULL);
    }
    template<typename T> T read(uintptr_t address)
    {
        T buffer{};
        readm((LPCVOID)address, &buffer, sizeof(T), NULL);

        return buffer;
    }

    void write_bytes(std::uint64_t address, std::vector<std::uint8_t> bytes) {
        SIZE_T bytesWritten;
        writem(reinterpret_cast<LPVOID>(address), (LPVOID)bytes.data(), bytes.size(), &bytesWritten);
    }

    auto read_bytes(unsigned long long address, unsigned long size) {
        std::vector<std::uint8_t> buffer(size);
        readm((LPCVOID)address, buffer.data(), size, NULL);
        return buffer;
    }

    bool suspendProcess();
    bool resumeProcess();
    bool suspend5Threads();
    bool resume5Threads();

};