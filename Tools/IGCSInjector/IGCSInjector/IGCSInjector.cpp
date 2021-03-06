#include "stdafx.h"
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <stdio.h>
#include <direct.h>
#include <sstream>
#include "CDataFile.h"
#include "Utils.h"

using namespace std;

#define INI_FILENAME	"IGCSInjector.ini"
#define VERSION			"1.0.1"

DWORD getProcessId(const WCHAR* targetProcessName);
string determineFullPathOfDll(string dllNameToInject);
int performInjection();
void displayHeader();
void displayError();

int main()
{
	displayHeader();
	int injectionResult = performInjection();

	system("pause");
	return injectionResult;
}

void displayHeader()
{
	cout << "Injectable Generic Camera System dll injector v" << VERSION << ", by Otis_Inf" << endl;
	cout << "Used for injecting IGCS based cameras into games" << endl;
	cout << "Source of IGCS, cameras and this exe can be found at GitHub:" << endl;
	cout << "https://github.com/FransBouma/InjectableGenericCameraSystem" << endl;
	cout << "------------------------------------------------------------------" << endl;
}


int performInjection()
{
	CDataFile iniFile(INI_FILENAME);
	string processName = iniFile.GetString("Process", "InjectionData");
	if (processName.length() <= 0)
	{
		cout << "Process name in the ini file isn't found. Exiting" << endl;
		return 1;
	}
	wstring targetProcessName(processName.begin(), processName.end());
	string dllNameToInject(iniFile.GetString("Dll", "InjectionData"));
	if (dllNameToInject.length() <= 0)
	{
		cout << "Dll name in the ini file isn't found. Exiting" << endl;
	}

	string fullPathOfDllToInject = determineFullPathOfDll(dllNameToInject);
	if (fullPathOfDllToInject.length() <= 0)
	{
		cout << "Couldn't determine the path / dll to inject. Exiting" << endl;
		return 1;
	}

	cout << "Dll to inject:\n" << fullPathOfDllToInject << endl;
	wcout << "Process to inject dll into:\n" << targetProcessName << endl;

	DWORD processId = getProcessId(targetProcessName.c_str());
	if (NULL == processId)
	{
		wcout << "Couldn't find process '" << targetProcessName.c_str() << "'. Exiting" << endl;
		return 1;
	}
	HANDLE processHandle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, processId);
	if (NULL == processHandle)
	{
		wcout << "Couldn't open the process '" << targetProcessName.c_str() << "' for injection. Did you start it as administrator? Exiting" << endl;
		displayError();
		return 1;
	}
	FARPROC loadLibraryAddress = GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	if (nullptr == loadLibraryAddress)
	{
		cout << "Couldn't obtain address for LoadLibraryA from kernel32.dll. Exiting" << endl;
		displayError();
		return 1;
	}
	LPVOID memoryInTargetProcess = VirtualAllocEx(processHandle, nullptr, fullPathOfDllToInject.length() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (nullptr == memoryInTargetProcess)
	{
		cout << "Couldn't allocate memory in the target process. Did you start it as administrator? Exiting" << endl;
		displayError();
		return 1;
	}
	SIZE_T amountBytesWritten;
	BOOL result = WriteProcessMemory(processHandle, memoryInTargetProcess, fullPathOfDllToInject.c_str(), fullPathOfDllToInject.length() + 1, &amountBytesWritten);
	if (!result)
	{
		cout << "Failed to write dll name into target process' memory space. Exiting" << endl;
		displayError();
		return 1;
	}

	HANDLE remoteThreadHandle = CreateRemoteThread(processHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddress), memoryInTargetProcess, 0, NULL);
	if (NULL == remoteThreadHandle)
	{
		cout << "Couldn't create a remote thread. Can't inject dll. Exiting." << endl;
		displayError();
		return 1;
	}

	VirtualFreeEx(processHandle, memoryInTargetProcess, fullPathOfDllToInject.length() + 1, MEM_RELEASE);
	CloseHandle(processHandle);

	cout << "Done! Enjoy!" << endl;
	return 0;
}


string determineFullPathOfDll(string dllNameToInject)
{
	char currentPath[MAX_PATH];
	if (!_getcwd(currentPath, MAX_PATH))
	{
		return "";
	}
	stringstream builder;
	builder << currentPath << "\\" << dllNameToInject;
	return builder.str();
}


DWORD getProcessId(const WCHAR* targetProcessName)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //all processes

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof entry;

	if (!Process32FirstW(snap, &entry))
	{
		return 0;
	}
	std::vector<DWORD> pids;
	do
	{
		if(_wcsicmp(entry.szExeFile, targetProcessName)==0)
		{
			HWND windowHandle = findMainWindow(entry.th32ProcessID);
			if (windowHandle > 0)
			{
				// has a main window.
				pids.emplace_back(entry.th32ProcessID);
			}
		}
	} while (Process32NextW(snap, &entry));

	if (pids.size() > 0)
	{
		return pids[0];
	}
	return NULL;
}

void displayError()
{
	DWORD errorCode = GetLastError();
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	string message(messageBuffer, size);

	cout << "Error code: " << (hex) << errorCode << endl;
	cout << "Error message: " << message << endl;

	LocalFree(messageBuffer);
}

