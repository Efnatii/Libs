#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>

#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <initializer_list>
#include <type_traits>

struct Module;
struct Process;
struct Window;
struct Thread;
struct Heap;

struct Module : private MODULEENTRY32
{
private:
public:
	inline BYTE* base_address() noexcept
	{
		return modBaseAddr;
	}
	inline DWORD size() noexcept
	{
		return modBaseSize;
	} 
	inline DWORD mid() noexcept
	{
		return th32ModuleID;
	}
	inline const WCHAR* name() noexcept
	{
		return szModule;
	}
	inline const WCHAR* path() noexcept
	{
		return szExePath;
	}
	
	inline Module(std::wstring wstrModuleName, DWORD dwProcessId)
	{
		void* Toolhelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);

		if (Toolhelp32Snapshot != INVALID_HANDLE_VALUE)
		{
			dwSize = sizeof(MODULEENTRY32);

			if (Module32First(Toolhelp32Snapshot, this))
			{
				do
				{
					if (wstrModuleName == szModule)
					{
						CloseHandle(Toolhelp32Snapshot);

						if (th32ModuleID)
							return;
					}
				} while (Module32Next(Toolhelp32Snapshot, this));
			}
		}

		throw std::runtime_error("\Module not found!");
	}

	Process get_process(DWORD dwDesiredAccess = PROCESS_ALL_ACCESS) noexcept;

	std::wstring to_string() noexcept
	{
		std::wstring wstrResult;

		wstrResult += L"[module]\n";

		wstrResult += L"module_struct_size=";
		wstrResult += std::to_wstring(dwSize);
		wstrResult += L"\n";

		wstrResult += L"module_size=";
		wstrResult += std::to_wstring(modBaseSize);
		wstrResult += L"\n";

		wstrResult += L"module_handle=";
		wstrResult += std::to_wstring((SIZE_T)hModule);
		wstrResult += L"\n";

		wstrResult += L"module_base_address=";
		wstrResult += std::to_wstring((SIZE_T)modBaseAddr);
		wstrResult += L"\n";
		
		wstrResult += L"module_name=\'";
		wstrResult += szModule;
		wstrResult += L"\'\n";

		wstrResult += L"module_path=\'";
		wstrResult += szExePath;
		wstrResult += L"\'\n";

		wstrResult += L"module_process_id=";
		wstrResult += std::to_wstring(th32ProcessID);
		wstrResult += L"\n";

		return wstrResult;
	}
};

struct Window
{
public:
	static inline Window get_current_console_window() noexcept
	{
		return { GetConsoleWindow() };
	}

private:
	DWORD dwSize;
	DWORD th32ThreadID;
	DWORD th32ProcessID;
	HWND hWindow;

public:
	inline Window(HWND hwndWindow)
	{
		dwSize = sizeof(Window);
		hWindow = hwndWindow;
		th32ThreadID = GetWindowThreadProcessId(hwndWindow, &th32ProcessID);
	}
	inline Window(DWORD dwProcessId)
	{
		dwSize = sizeof(Window);
		th32ProcessID = dwProcessId;

		if (EnumWindows(
			[](HWND hwnd, LPARAM lparam)
			{
				Window& window = *(Window*)lparam;
				DWORD dwProcessId;

				window.th32ThreadID = GetWindowThreadProcessId(hwnd, &dwProcessId);

				if (dwProcessId == window.th32ProcessID)
				{
					window.hWindow = hwnd;

					return 0;
				}

				return 1;
			}
			, (LPARAM)this))
			throw std::runtime_error("\Window not found!");
	}

	inline int minimize() noexcept
	{
		return CloseWindow(hWindow);
	}

	inline std::wstring text() noexcept
	{
		auto length = GetWindowTextLength(hWindow) + 1;
		std::wstring wstrResult(length, L'0');

		GetWindowText(hWindow, (WCHAR*)wstrResult.c_str(), length);

		return wstrResult;
	}

public:
	std::wstring to_string() noexcept
	{
		std::wstring wstrResult;

		wstrResult += L"[window]\n";

		wstrResult += L"window_struct_size=";
		wstrResult += std::to_wstring(dwSize);
		wstrResult += L"\n";

		wstrResult += L"window_handle=";
		wstrResult += std::to_wstring((SIZE_T)hWindow);
		wstrResult += L"\n";

		wstrResult += L"window_text=\'";
		wstrResult += text();
		wstrResult += L"\'\n";

		wstrResult += L"window_thread_id=";
		wstrResult += std::to_wstring(th32ThreadID);
		wstrResult += L"\n";

		wstrResult += L"window_process_id=";
		wstrResult += std::to_wstring(th32ProcessID);
		wstrResult += L"\n";

		return wstrResult;
	}
};


struct Process : private PROCESSENTRY32
{
public:
	static inline Process get_current_process(DWORD dwDesiredAccess = PROCESS_ALL_ACCESS) noexcept
	{
		return { GetCurrentProcessId(), dwDesiredAccess };
	}

private:
	HANDLE hProcess;

public:
	inline long base_thread_priority()
	{
		return pcPriClassBase;
	}
	inline DWORD thread_count()
	{
		return cntThreads;
	}
	inline DWORD pid()
	{
		return th32ProcessID;
	}
	inline const WCHAR* name()
	{
		return szExeFile;
	}

	inline Process parent_process(DWORD dwDesiredAccess = PROCESS_ALL_ACCESS)
	{
		return { th32ParentProcessID, dwDesiredAccess };
	}

public:
	inline Process(DWORD dwProcessId, DWORD dwDesiredAccess = PROCESS_ALL_ACCESS) 
	{
		void* Toolhelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		
		if (Toolhelp32Snapshot != INVALID_HANDLE_VALUE)
		{
			dwSize = sizeof(PROCESSENTRY32);

			if (Process32First(Toolhelp32Snapshot, this))
			{
				do
				{
					if (dwProcessId == th32ProcessID)
					{
						CloseHandle(Toolhelp32Snapshot);

						if (th32ProcessID)
						{
							if (hProcess = OpenProcess(dwDesiredAccess, false, th32ProcessID))
								return;
						}
					}
				} while (Process32Next(Toolhelp32Snapshot, this));
			}
		}

		throw std::runtime_error("\Process not found!");
	}
	inline Process(std::wstring wstrProcessName, DWORD dwDesiredAccess = PROCESS_ALL_ACCESS)
	{
		void* Toolhelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if (Toolhelp32Snapshot != INVALID_HANDLE_VALUE)
		{
			dwSize = sizeof(PROCESSENTRY32);

			if (Process32First(Toolhelp32Snapshot, this))
			{
				do
				{
					if (wstrProcessName == szExeFile)
					{
						CloseHandle(Toolhelp32Snapshot);

						if (th32ProcessID)
						{
							if (hProcess = OpenProcess(dwDesiredAccess, false, th32ProcessID))
								return;
						}
					}
				} while (Process32Next(Toolhelp32Snapshot, this));
			}		
		}

		throw std::runtime_error("\Process not found!");
	}
	
	inline ~Process()
	{
		CloseHandle(hProcess);
	}

public:
	template<class T> inline void read(LPCVOID lpBaseAddress, T& tBuffer) noexcept
	{
		ReadProcessMemory(hProcess, lpBaseAddress, &tBuffer, sizeof(T), nullptr);
	}
	template<class T, SIZE_T Sz> inline void read(LPCVOID lpBaseAddress, T (&tBuffer)[Sz]) noexcept
	{
		ReadProcessMemory(hProcess, lpBaseAddress, &tBuffer, sizeof(T) * Sz, nullptr);
	}
	template<template <typename...> class X, class T, class... Ts> inline void read(LPCVOID lpBaseAddress, X<T, Ts...>& tBuffer) noexcept
	{
		ReadProcessMemory(hProcess, lpBaseAddress, tBuffer.data(), sizeof(T) * tBuffer.size(), nullptr);
	}

	template<class T> inline void write(LPVOID lpBaseAddress, T&& tBuffer)
	{
		WriteProcessMemory(hProcess, lpBaseAddress, &tBuffer, sizeof(T), nullptr);
	}
	template<class T, unsigned long long Sz> inline void write(LPVOID lpBaseAddress, T(&&tBuffer)[Sz]) noexcept
	{
		WriteProcessMemory(hProcess, lpBaseAddress, &tBuffer, sizeof(T) * Sz, nullptr);
	}
	template<template <typename...> class X, class T, class... Ts> inline void write(LPVOID lpBaseAddress, X<T, Ts...>&& tBuffer)
	{
		WriteProcessMemory(hProcess, lpBaseAddress, tBuffer.data(), sizeof(T) * tBuffer.size(), nullptr);
	}

	inline LPVOID find_pointer(Module& lModule, SIZE_T tOffset) noexcept
	{
		return lModule.base_address() + tOffset;
	}
	template<SIZE_T nSize> inline LPVOID find_pointer(Module& lModule, SIZE_T(&&tOffsets)[nSize]) noexcept
	{
		BYTE* lpBaseAddress = lModule.base_address();

		if (nSize)
		{
			for (SIZE_T index = 0; index < (nSize - 1); index++)
				read(lpBaseAddress + tOffsets[index], lpBaseAddress);

			return lpBaseAddress + tOffsets[nSize - 1];
		}

		return lpBaseAddress;
	}
	template<template <typename...> class X, class T, class... Ts> inline LPVOID find_pointer(Module& lModule, X<T, Ts...>&& tOffsets) noexcept
	{
		BYTE* lpBaseAddress = lModule.base_address();

		if (tOffsets.size())
		{
			for (SIZE_T index = 0; index < (tOffsets.size() - 1); index++)
				read(lpBaseAddress + tOffsets[index], lpBaseAddress);

			return lpBaseAddress + tOffsets[tOffsets.size() - 1];
		}

		return lpBaseAddress;
	}

public:
	inline Module get_module(std::wstring ModuleName) noexcept;
	inline Module get_module() noexcept;

	inline Window get_window() noexcept;

public:
	std::wstring to_string() noexcept
	{
		std::wstring wstrResult;

		wstrResult += L"[process]\n";

		wstrResult += L"process_struct_size=";
		wstrResult += std::to_wstring(dwSize);
		wstrResult += L"\n";

		wstrResult += L"process_handle=";
		wstrResult += std::to_wstring((SIZE_T)hProcess);
		wstrResult += L"\n";

		wstrResult += L"process_name=\'";
		wstrResult += szExeFile;
		wstrResult += L"\'\n";

		wstrResult += L"process_id=";
		wstrResult += std::to_wstring(th32ProcessID);
		wstrResult += L"\n";

		wstrResult += L"process_parent_process_id=";
		wstrResult += std::to_wstring(th32ParentProcessID);
		wstrResult += L"\n";

		wstrResult += L"process_thread_count=";
		wstrResult += std::to_wstring(cntThreads);
		wstrResult += L"\n";

		wstrResult += L"process_base_thread_priority=";
		wstrResult += std::to_wstring(pcPriClassBase);
		wstrResult += L"\n";

		return wstrResult;
	}
};

inline Module Process::get_module(std::wstring ModuleName) noexcept
{
	return { ModuleName, th32ProcessID };
}
inline Module Process::get_module() noexcept
{
	return { szExeFile, th32ProcessID };
}

inline Window Process::get_window() noexcept
{
	return { th32ProcessID };
}

inline Process Module::get_process(DWORD dwDesiredAccess) noexcept
{
	return { th32ProcessID, dwDesiredAccess };
}

