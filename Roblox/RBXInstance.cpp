#include <iostream>
#include "RBXInstance.hpp"
#include "driver/Driver.h"
#include "UserMode/Memory.hpp"
#include "xxhash.h"
#include "zstd.h"
#include "RBXBytecode.hpp"
#include "offsets.hpp"

extern "C" NTSTATUS NTAPI NtReadVirtualMemory(
        IN HANDLE ProcessHandle,
        IN PVOID BaseAddress,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT PULONG NumberOfBytesRead OPTIONAL
);

extern "C" NTSTATUS NtUnlockVirtualMemory(
        HANDLE ProcessHandle,
        PVOID* BaseAddress,
        PSIZE_T RegionSize,
        ULONG UnlockType
);

const auto pBytecode{RBXBytecode::get_singleton() };
const auto pDriver{ Driver::get_singleton() };
const auto pMemory { Memory::get_singleton() };

std::string readstring(std::uint64_t address)
{
    std::string string;
    char character = 0;
    int char_size = sizeof(character);
    int offset = 0;

    string.reserve(204);

    while (offset < 200)
    {
        character = pMemory->read<char>(address + offset);

        if (character == 0)
            break;

        offset += char_size;
        string.push_back(character);
    }

    return string;
}


std::string readstring2(std::uint64_t string)
{
    const auto length = pMemory->read<int>(string + 0x18);

    if (length >= 16u)
    {
        const auto New = pMemory->read<std::uint64_t>(string);
        return readstring(New);
    }
    else
    {
        const auto Name = readstring(string);
        return Name;
    }
}

std::string readstring3(std::uint64_t string, int length)
{
    if (length >= 16u)
    {
        const auto New = pMemory->read<std::uint64_t>(string);
        return readstring(New);
    }
    else
    {
        const auto Name = readstring(string);
        return Name;
    }
}

std::string RBXInstance::name()
{
    const auto ptr = pMemory->read<std::uint64_t>(this->self + offsets::Name);

    if (ptr)
        return readstring2(ptr);

    return "???";
}

std::string RBXInstance::class_name()
{
    const auto ptr = pMemory->read<std::uint64_t>(this->self + offsets::Classname);

    if (ptr)
        return readstring(pMemory->read<std::uint64_t>(ptr + 0x8));

    return "???_classname";
}

std::uint64_t RBXInstance::get_class_descriptor()
{
    return pMemory->read<std::uint64_t>(this->self + offsets::Classname);
}


std::vector<RBXInstance> RBXInstance::children()
{
    std::vector<RBXInstance> container;

    if (!this->self)
        return container;

    auto start = pMemory->read<std::uint64_t>(this->self + offsets::Children);

    if (!start)
        return container;

    auto end = pMemory->read<std::uint64_t>(start + offsets::Size);

    for (auto instances = pMemory->read<std::uint64_t>(start); instances != end; instances += 16)
        container.emplace_back(pMemory->read<RBXInstance>(instances));

    return container;
}

RBXInstance RBXInstance::FindFirstChild(std::string child)
{
    RBXInstance ret;

    for (auto& object : this->children())
    {
        if (object.name() == child)
        {
            ret = static_cast<RBXInstance>(object);
            break;
        }
    }

    return ret;
}

RBXInstance RBXInstance::FindFirstChildOfClass(std::string class_name)
{
    RBXInstance ret;

    for (auto& object : this->children())
    {

        if (object.class_name() == class_name)
        {
            ret = static_cast<RBXInstance>(object);
            break;
        }
    }

    return ret;
}

void RBXInstance::SetBytecode(std::vector<uint8_t> bytes, int bytecode_size) {
    if (this->class_name() != "LocalScript" && this->class_name() != "ModuleScript")
        return;

    std::uintptr_t embeddedPtr = pMemory->read<std::uintptr_t>(this->self + offsets::ModuleScriptByteCode);

    auto allocatedAddress = reinterpret_cast<std::uintptr_t>(pMemory->allocate(bytecode_size));
    if (allocatedAddress == 0)
        return;

    pMemory->write_bytes(allocatedAddress, bytes);

    pMemory->write<std::uintptr_t>(embeddedPtr + 0x10, allocatedAddress);
    pMemory->write<std::uint64_t>(embeddedPtr + 0x20, bytecode_size);
}

std::string RBXInstance::Bytecode()
{
    auto class_name = this->class_name();

    int offset = 0x0;

    if (class_name == "ModuleScript") {
        offset = offsets::ModuleScriptByteCode;
    }
    else {
        throw std::runtime_error("Invalid Script");
    }

    std::cout << std::hex << this->self << std::endl;
    auto bytecode_ptr = pMemory->read<std::uintptr_t>(this->self + offset);

    auto bytecode_ptr_str = pMemory->read<uintptr_t>(bytecode_ptr + 0x10);
    auto bytecode_size = pMemory->read<uint64_t>(bytecode_ptr + 0x20);

    std::string bytecodeBuffer;
    bytecodeBuffer.resize(bytecode_size);

    std::cout << readstring3(bytecode_ptr_str, bytecode_size) << std::endl;

    MEMORY_BASIC_INFORMATION bi;
    VirtualQueryEx(pMemory->RobloxProcess, reinterpret_cast<LPCVOID>(bytecode_ptr_str), &bi, sizeof(bi));

    ULONG bytesRead;
    NTSTATUS status = NtReadVirtualMemory(pMemory->RobloxProcess,reinterpret_cast<PVOID>(bytecode_ptr_str),bytecodeBuffer.data(),static_cast<ULONG>(bytecode_size),&bytesRead);

    PVOID baddr = bi.AllocationBase;
    SIZE_T size = bi.RegionSize;
    NtUnlockVirtualMemory(pMemory->RobloxProcess, &baddr, &size, 1);

    return pBytecode->Decompress(bytecodeBuffer);
}
