/*
 * process.h
 *
 *  Created on: 6 янв. 2024 г.
 *      Author: Efnatii
 */

#ifndef INCLUDE_MEMORY_PROCESS_H_
#define INCLUDE_MEMORY_PROCESS_H_


#define PROCESS_CREATE_PROCESS  				0x0080
#define PROCESS_CREATE_THREAD  					0x0002
#define PROCESS_DUP_HANDLE  					0x0040
#define PROCESS_QUERY_INFORMATION  				0x0400
#define PROCESS_QUERY_LIMITED_INFORMATION  		0x1000
#define PROCESS_SET_INFORMATION  				0x0200
#define PROCESS_SET_QUOTA  						0x0100
#define PROCESS_SUSPEND_RESUME  				0x0800
#define PROCESS_TERMINATE  						0x0001
#define PROCESS_VM_OPERATION  					0x0008
#define PROCESS_VM_READ  						0x0010
#define PROCESS_VM_WRITE  						0x0020
#define SYNCHRONIZE  							0x00100000L

#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )

#define TH32CS_INHERIT 							0x80000000
#define TH32CS_SNAPHEAPLIST 					0x00000001
#define TH32CS_SNAPMODULE						0x00000008
#define TH32CS_SNAPMODULE32 					0x00000010
#define TH32CS_SNAPPROCESS						0x00000002
#define TH32CS_SNAPTHREAD						0x00000004
#define TH32CS_SNAPALL							H32CS_SNAPHEAPLIST | TH32CS_SNAPMODULE | TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD

struct PROCESSENTRY32;
struct MODULEENTRY32;

extern "C" {
DllImport void* __stdcall OpenProcess (unsigned long dwDesiredAccess, int bInheritHandle, unsigned long dwProcessId);

DllImport int ReadProcessMemory(void* hProcess, unsigned long long int lpBaseAddress, void* buffer, unsigned long long size, unsigned long long* lpNumberOfBytesRead);
DllImport int WriteProcessMemory(void* hProcess, unsigned long long int lpBaseAddress, void* buffer, unsigned long long size, unsigned long long* lpNumberOfBytesWritten);

DllImport void* CreateToolhelp32Snapshot(unsigned long dwFlags, unsigned long th32ProcessID);

DllImport int Process32First(void* hToolhelp32Snapshot, PROCESSENTRY32* lppe);
DllImport int Process32Next(void* hToolhelp32Snapshot, PROCESSENTRY32* lppe);
DllImport int Module32First(void* hToolhelp32Snapshot, MODULEENTRY32* lpme);
DllImport int Module32Next(void* hToolhelp32Snapshot, MODULEENTRY32* lppme);

DllImport int CloseHandle (void* hObject);
}

struct PROCESSENTRY32 {
	unsigned long     			dwSize;
	unsigned long     			cntUsage;
	unsigned long     			th32ProcessID;
  	unsigned long long* 		th32DefaultHeapID;
  	unsigned long     			th32ModuleID;
  	unsigned long     			cntThreads;
  	unsigned long     			th32ParentProcessID;
  	int      					pcPriClassBase;
  	unsigned long     			dwFlags;
  	char      					szExeFile[260];
};

struct MODULEENTRY32 {
	unsigned long   			dwSize;
	unsigned long   			th32ModuleID;
	unsigned long  				th32ProcessID;
	unsigned long  				GlblcntUsage;
	unsigned long  				ProccntUsage;
	void*						modBaseAddr;
	unsigned long   			modBaseSize;
	void* 						hModule;
	char    					szModule[256];
	char    					szExePath[260];
};

#include <string>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <initializer_list>


class Module;
class Process;;
class DisplacedPointer;

class Module
{
	MODULEENTRY32		ModuleEntry;

public:
	inline unsigned long long int GetBaseAddress()
	{
		return (unsigned long long int)ModuleEntry.modBaseAddr;
	}
	inline MODULEENTRY32* GetModuleEntry()
	{
		return &ModuleEntry;
	}
	inline unsigned long GetModuleId()
	{
		return ModuleEntry.th32ModuleID;
	}
	std::string GetModuleName()
	{
		return ModuleEntry.szModule;
	}

	Module()
	{
		std::memset(&ModuleEntry, 0, sizeof(MODULEENTRY32));
		ModuleEntry.dwSize = sizeof(MODULEENTRY32);
	}
};


class Process
{
	PROCESSENTRY32 		ProcessEntry;
	void*				ProcessHandle = nullptr;

	unsigned long long 	byteswrite;
	unsigned long long 	bytesread;

public:
	Process() = default;

	inline unsigned long GetProcessId()
	{
		return ProcessEntry.th32ProcessID;
	}
	inline void* GetProcessHandle()
	{
		return ProcessHandle;
	}
	inline char* GetProcessName()
	{
		return ProcessEntry.szExeFile;
	}

	inline Process(std::string ProcessName, unsigned long Access = 0x000F0000 | 0x00100000 | 0xffff);

	template<typename T> inline T Read(unsigned long long int Address);
	template<typename T> inline T* Read(unsigned long long int Address, unsigned long long int Length);

	template<typename T> inline void Write(unsigned long long int Address, T Value);
	template<typename T> inline void Write(unsigned long long int Address, void* Buffer, unsigned long long int Length);

	inline Module GetModule(std::string ModuleName);
};

class DisplacedPointer
{
protected:
	Process* 								_Process = nullptr;
	Module* 								_Module	 = nullptr;

public:
	inline unsigned long GetAddress(std::initializer_list<unsigned long> _Offsets)
	{
		const unsigned long* 							p_Offsets = _Offsets.begin();
		const unsigned long long int 					_Length = _Offsets.size();

		unsigned long _Address = _Module->GetBaseAddress();

		for (unsigned long long int index = 0; index < (_Length - 1); index++)
		{
			//std::cout << std::hex << _Address << std::dec << std::endl;
			_Address = _Process->Read<unsigned long>(_Address + p_Offsets[index]);
		}
		//std::cout << std::hex << _Address << std::dec << std::endl;
		_Address = _Address + p_Offsets[_Length - 1];

		return _Address;
	}

	inline DisplacedPointer() = default;
	inline DisplacedPointer(Process* _Process, Module* _Module)
	{
		this->_Process = _Process;
		this->_Module = _Module;
	}

	virtual void Update() = 0;
};

inline Process::Process(std::string ProcessName, unsigned long Access)
{
	void* Toolhelp32Snapshot = nullptr;
	Toolhelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	std::memset(&ProcessEntry, 0, sizeof(PROCESSENTRY32));
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(Toolhelp32Snapshot, &ProcessEntry))
	{
		do
		{
			if (std::string(ProcessEntry.szExeFile) == ProcessName)
				break;
		} while (Process32Next(Toolhelp32Snapshot, &ProcessEntry));
	}
	else throw std::runtime_error("\n Toolhelp32SnapshotError:\n\tProcess32First returned zero!");

	CloseHandle(Toolhelp32Snapshot);

	if (ProcessEntry.th32ProcessID)
	{
		ProcessHandle = OpenProcess(Access, false, ProcessEntry.th32ProcessID);
	}
	else throw std::runtime_error("\n  ProcessError:\n\tProcessId is zero!");
}

template<typename T>
inline T Process::Read(unsigned long long int Address)
{
	T buffer;
	ReadProcessMemory(GetProcessHandle(), Address, &buffer, sizeof(T), &bytesread);
	return buffer;
}

template<typename T>
inline T* Process::Read(unsigned long long int Address, unsigned long long int Length)
{
	T* buffer = new T[Length];
	ReadProcessMemory(GetProcessHandle(), Address, buffer, sizeof(T) * Length, &bytesread);
	return buffer;
}

template<typename T>
inline void Process::Write(unsigned long long int Address, T Value)
{
	WriteProcessMemory(GetProcessHandle(), Address, &Value, sizeof(T), &byteswrite);
}

template<typename T>
inline void Process::Write(unsigned long long int Address, void* Buffer, unsigned long long int Length)
{
	WriteProcessMemory(GetProcessHandle(), Address, Buffer, sizeof(T) * Length, &byteswrite);
}

inline Module Process::GetModule(std::string ModuleName)
{
	void* Toolhelp32Snapshot = nullptr;
	Module module;

	Toolhelp32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId());

	if (Module32First(Toolhelp32Snapshot, module.GetModuleEntry()))
	{
		do
		{
			if (module.GetModuleName() == ModuleName)
				break;
		} while (Module32Next(Toolhelp32Snapshot, module.GetModuleEntry()));
	}
	else throw std::runtime_error("\n  Toolhelp32SnapshotError:\n\tModule32First returned zero!");

	CloseHandle(Toolhelp32Snapshot);

	if (module.GetModuleId())
		return module;
	else throw std::runtime_error("\n  ModuleError:\n\tModuleId is zero!");
}

#endif /* INCLUDE_MEMORY_PROCESS_H_ */
