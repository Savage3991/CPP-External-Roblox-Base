#include "RBXDataModel.hpp"
#include <iostream>
#include "Roblox/RBXInstance.hpp"
#include <string>
#include <locale>
#include <codecvt>
#include "../../driver/Driver.h"
#include "utils/utils.hpp"
#include "../offsets.hpp"

uintptr_t CustomGetViewRegex(const std::wstring& folderPath, const std::wstring& latestFile) {
    std::wregex regexPattern(L"view\\((\\w+)\\)");
    std::wifstream fileStream(folderPath + L"\\" + latestFile);
    std::wstring line;
    std::wsmatch match;
    uintptr_t newAddress = 0;
    while (std::getline(fileStream, line) && !std::regex_search(line, match, regexPattern));
    std::wstringstream ss;
    ss << std::hex << match[1];
    ss >> newAddress;
    return newAddress;
}

std::wstring CustomGetLatestFile(const std::wstring& folderPath) {
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((folderPath + L"\\*").c_str(), &findFileData);
    std::wstring latestFile;
    FILETIME ftLatest = { 0, 0 };
    while (hFind != INVALID_HANDLE_VALUE && FindNextFileW(hFind, &findFileData)) {
        if (CompareFileTime(&ftLatest, &findFileData.ftLastWriteTime) < 0) {
            ftLatest = findFileData.ftLastWriteTime;
            latestFile = findFileData.cFileName;
        }
    }
    FindClose(hFind);
    return latestFile;
}

const auto pDriver{ Driver::get_singleton() };

RBXDataModel* RBXDataModel::g_Singleton = nullptr;

RBXDataModel* RBXDataModel::get_singleton() noexcept {
    if (g_Singleton == nullptr)
        g_Singleton = new RBXDataModel();
    return g_Singleton;
}

std::uint64_t RBXDataModel::get_renderView()
{
    TCHAR localAppData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("LOCALAPPDATA"), localAppData, MAX_PATH) > 0) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring logsPath = converter.from_bytes(localAppData) + L"\\Roblox\\logs";
        uintptr_t ViewRegex = CustomGetViewRegex(logsPath.c_str(), CustomGetLatestFile(logsPath.c_str()));

        return ViewRegex;
    }
}

std::uint64_t RBXDataModel::get_datamodel()
{
    unsigned long long DataModelAddress = 0;
    unsigned long long RenderViewAddress = 0;

    TCHAR localAppData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("LOCALAPPDATA"), localAppData, MAX_PATH) > 0) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring logsPath = converter.from_bytes(localAppData) + L"\\Roblox\\logs";
        uintptr_t ViewRegex = CustomGetViewRegex(logsPath.c_str(), CustomGetLatestFile(logsPath.c_str()));

        auto DataModelHolder = pDriver->read<uint64_t>(ViewRegex + offsets::DataModelHolder);
        if (!DataModelHolder)
        {
            return NULL;
        }

        auto DataModel = pDriver->read<uint64_t>(DataModelHolder + offsets::DataModel);
        if (!DataModel)
        {
            return NULL;
        }

        this->datamodel = DataModel;
    }

    return this->datamodel;
}