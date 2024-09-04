#pragma once
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <regex>
#include <Windows.h>
#include <fstream>
#include <sstream>

namespace utils
{
	inline HWND hwndout;
	inline BOOL EnumWindowProcMy(HWND input, LPARAM lParam)
	{

		DWORD lpdwProcessId;
		GetWindowThreadProcessId(input, &lpdwProcessId);
		if (lpdwProcessId == lParam)
		{
			hwndout = input;
			return FALSE;
		}
		return true;
	}
	inline HWND get_hwnd_of_process_id(int target_process_id)
	{
		EnumWindows(EnumWindowProcMy, target_process_id);
		return hwndout;
	}
	inline std::string replace(std::string subject, std::string search, std::string replace) {
		size_t pos = 0;

		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}

		return subject;
	}
	inline int get_pid_from_name(const char* name)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);
		Process32First(snapshot, &entry);
		do
		{
			if (strcmp(entry.szExeFile, name) == 0)
			{
				return entry.th32ProcessID;
			}

		} while (Process32Next(snapshot, &entry));

		return 0; // if not found
	}

    inline auto read_file(const std::string& file_location) -> std::string {
        auto close_file = [](FILE* f) { fclose(f); };
        auto holder = std::unique_ptr<FILE, decltype(close_file)>(fopen(file_location.c_str(), "rb"), close_file);

        if (!holder)
            return "";

        FILE* f = holder.get();

        if (fseek(f, 0, SEEK_END) < 0)
            return "";

        const long size = ftell(f);

        if (size < 0)
            return "";

        if (fseek(f, 0, SEEK_SET) < 0)
            return "";

        std::string res;
        res.resize(size);
        fread(const_cast<char*>(res.data()), 1, size, f);

        return res;
    }

    inline std::string read_file2(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Unable to open file: " + filePath);
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF) {
            content = content.substr(3);
        }

        return content;
    }

    inline std::vector<unsigned char> combine(const std::vector<unsigned char>& v1, const std::vector<unsigned char>& v2) {
        std::vector<unsigned char> combined = v1;
        combined.insert(combined.end(), v2.begin(), v2.end());
        return combined;
    }

    inline std::vector<unsigned char> to_bytes(uintptr_t value, size_t num_bytes) {
        std::vector<unsigned char> bytes(num_bytes);
        for (size_t i = 0; i < num_bytes; ++i) {
            bytes[i] = value & 0xFF;
            value >>= 8;
        }

        return bytes;
    }
}
