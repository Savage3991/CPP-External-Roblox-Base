#include <iostream>

#include <thread>
#include "Roblox/DataModel/RBXDataModel.hpp"
#include "Roblox/RBXInstance.hpp"
#include "utils/utils.hpp"
#include <zstd.h>
#include <xxhash.h>
#include "Roblox/RBXBytecode.hpp"
#include "UserMode/Memory.hpp"
#include "UserMode/unhook.hpp"
#include <filesystem>
#include "Driver/Driver.h"
#include "Mapper/kdmapper.hpp"
#include "Mapper/utils.hpp"
#include "Mapper/driver.h"

#define JobObjectFreezeInformation ((JOBOBJECTINFOCLASS)18)

typedef struct _JOBOBJECT_WAKE_FILTER
{
    ULONG HighEdgeFilter;
    ULONG LowEdgeFilter;
} JOBOBJECT_WAKE_FILTER, *PJOBOBJECT_WAKE_FILTER;

typedef struct _JOBOBJECT_FREEZE_INFORMATION
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG FreezeOperation : 1;
            ULONG FilterOperation : 1;
            ULONG SwapOperation : 1;
            ULONG Reserved : 29;
        };
    };
    BOOLEAN Freeze;
    BOOLEAN Swap;
    UCHAR Reserved0[2];
    JOBOBJECT_WAKE_FILTER WakeFilter;
} JOBOBJECT_FREEZE_INFORMATION, *PJOBOBJECT_FREEZE_INFORMATION;

const auto pBytecode{RBXBytecode::get_singleton() };
const auto pDatamodel{RBXDataModel::get_singleton() };

HANDLE iqvw64e_device_handle;
using namespace kdmapper;
using namespace intel_driver;

int main() {
    std::cout << "Initializing..." << std::endl;

    HANDLE device_handler = Load();

    if (!device_handler || device_handler == INVALID_HANDLE_VALUE)
    {
        std::cout << "Failed to Initialize Driver" << std::endl;
        std::cin.get();
        return -1;
    }

    MapDriverBytes(device_handler, RawData);

    Unload(device_handler);

    std::cout << "Driver Initialized" << std::endl;

    std::cout << "Searching Roblox" << std::endl;
    const auto pDriver{ Driver::get_singleton() };
    const auto pMemory{Memory::get_singleton()};

    std::wstring target = L"RobloxPlayerBeta.exe";
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &pe32)) {
            do {
                if (strcmp(pe32.szExeFile, "RobloxPlayerBeta.exe") == 0) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    if (pid == 0) {
        std::cout << "Couldnt find Roblox" << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "Found Roblox" << std::endl;

    pDriver->initialize(L"\\\\.\\{f751dd83-fcc5-43b5-aa0d-398fe67bc306}", pid);

    uintptr_t target_process_base_address = pDriver->get_base_address(L"RobloxPlayerBeta.exe");

    if (target_process_base_address == 0) {
        std::cout << "Failed to get base address of roblox. This may be because of the driver not being loaded" << std::endl;
    }

    auto target_process_hwnd = utils::get_hwnd_of_process_id(pid);
    auto thread_id = GetWindowThreadProcessId(target_process_hwnd, 0);

    std::cout << "------------------------------------------------------------------" << std::endl;

    std::cout << "Trying to grab DataModel" << std::endl;
    std::uint64_t datamodel = pDatamodel->get_datamodel();
    std::cout << "Got DataModel" << std::endl;

    std::cout << "------------------------------------------------------------------" << std::endl;

    auto compressedENV = pBytecode->compile_to_bytecode(utils::read_file("ENV.lua"));

    std::vector<uint8_t> LuauENV(compressedENV.begin(), compressedENV.end());

    std::cout << "Compiled ENV" << std::endl;

    std::cout << "------------------------------------------------------------------" << std::endl;

    std::cout << "Initializing Usermode Operations" << std::endl;

    unhookkernel("VirtualFree");
    unhookkernel("VirtualFreeEx");
    unhookkernel("VirtualProtect");
    unhookkernel("VirtualProtectEx");
    unhookkernel("WriteProcessMemory");
    unhookkernel("ReadProcessMemory");
    unhookntdll("NtUnlockVirtualMemory");
    pMemory->init_NTfunctions();

    if (!pMemory->setup(pid)) {
        std::cout << "Failed to Initialize Usermode Memory" << std::endl;
        exit(0);
    }

    std::cout << "------------------------------------------------------------------" << std::endl;

    RBXInstance game = static_cast<RBXInstance>(datamodel);
    std::cout << "DataModel: 0x" << std::hex << game.self << std::endl;
    std::cout << "DataModel Type: " << game.name() << std::endl;

    auto CoreGui = game.FindFirstChildOfClass("CoreGui");
    std::cout << "CoreGui: 0x" << std::hex << CoreGui.self << std::endl;

    auto RobloxGui = CoreGui.FindFirstChild("RobloxGui");
    std::cout << "RobloxGui: 0x" << std::hex << RobloxGui.self << std::endl;

    auto Modules = RobloxGui.FindFirstChild("Modules");
    std::cout << "Modules: 0x" << std::hex << Modules.self << std::endl;

    auto Common = Modules.FindFirstChild("Common");
    std::cout << "Common: 0x" << std::hex << Common.self << std::endl;

    auto HomePageScriptLoader = Common.FindFirstChild("PolicyService");
    std::cout << "HomePage ScriptLoader: 0x" << std::hex << HomePageScriptLoader.self << std::endl;

    std::cout << "------------------------------------------------------------------" << std::endl;

    HomePageScriptLoader.SetBytecode(LuauENV, compressedENV.size());

}
