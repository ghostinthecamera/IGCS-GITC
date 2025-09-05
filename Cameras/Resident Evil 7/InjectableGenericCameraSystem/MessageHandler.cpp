////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2020, Frans Bouma
// All rights reserved.
// https://github.com/FransBouma/InjectableGenericCameraSystem
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MessageHandler.h"
#include "NamedPipeManager.h"
#include "Utils.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace IGCS::MessageHandler
{
	// Static variables for file logging
	static std::mutex s_logMutex;
	static std::ofstream s_logFile;
	static bool s_fileLoggingEnabled = false;
	static std::string s_logFilePath;

	// Internal helper functions
	std::string getCurrentTimestamp()
	{
		auto now = std::chrono::system_clock::now();
		auto time_t = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
		localtime_s(&tm, &time_t);

		std::stringstream ss;
		ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	void writeToLogFile(const std::string& message, const std::string& level)
	{
		if (!s_fileLoggingEnabled || !s_logFile.is_open())
			return;

		std::lock_guard<std::mutex> lock(s_logMutex);
		s_logFile << "[" << getCurrentTimestamp() << "] [" << level << "] " << message << std::endl;
		s_logFile.flush(); // Ensure immediate write to disk
	}

	// Public functions to manage file logging
	bool initializeFileLogging(const std::string& logFileName, bool clearFile)
	{
		std::lock_guard<std::mutex> lock(s_logMutex);

		// Close existing log file if open
		if (s_logFile.is_open())
		{
			s_logFile.close();
		}

		// Get DLL path instead of executable path
		HMODULE hModule = nullptr;
		// Get handle to this DLL by using a function address from this code
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(&initializeFileLogging),
			&hModule);

		char dllPath[MAX_PATH] = { 0 };
		GetModuleFileNameA(hModule, dllPath, MAX_PATH);

		std::string dllDir = dllPath;
		size_t lastSlash = dllDir.find_last_of("\\/");
		if (lastSlash != std::string::npos)
		{
			dllDir = dllDir.substr(0, lastSlash + 1);
		}

		s_logFilePath = dllDir + logFileName;

		// Open log file - either truncate (clear) or append based on parameter
		auto openMode = clearFile ? (std::ios::out | std::ios::trunc) : (std::ios::out | std::ios::app);
		s_logFile.open(s_logFilePath, openMode);

		if (s_logFile.is_open())
		{
			s_fileLoggingEnabled = true;
			s_logFile << "=== Log session started at " << getCurrentTimestamp() << " ===" << std::endl;
			s_logFile << "DLL Path: " << dllPath << std::endl;
			return true;
		}

		s_fileLoggingEnabled = false;
		return false;
	}

	void shutdownFileLogging()
	{
		std::lock_guard<std::mutex> lock(s_logMutex);
		if (s_logFile.is_open())
		{
			s_logFile << "=== Log session ended at " << getCurrentTimestamp() << " ===" << std::endl;
			s_logFile.close();
		}
		s_fileLoggingEnabled = false;
	}

	void enableFileLogging(bool enable)
	{
		s_fileLoggingEnabled = enable;
	}

	bool isFileLoggingEnabled()
	{
		return s_fileLoggingEnabled;
	}

	// New function to write directly to log file
	void logToFile(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

		writeToLogFile(formattedArgs, "INFO");
	}

	void logDebugToFile(const char* fmt, ...)
	{
#ifdef _DEBUG
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

		writeToLogFile(formattedArgs, "DEBUG");
#endif
	}

	void logErrorToFile(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

		writeToLogFile(formattedArgs, "ERROR");
	}

	// Original functions with added file logging
	void addNotification(const string& notificationText)
	{
		NamedPipeManager::instance().writeNotification(notificationText);
		writeToLogFile(notificationText, "NOTIFICATION");
	}

	void addNotificationOnly(const std::string& notificationText)
	{
		NamedPipeManager::instance().writeNotificationOnly(notificationText);
		writeToLogFile(notificationText, "NOTIFICATION");
	}

	void logDebug(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);
		writeToLogFile(formattedArgs, "DEBUG");
#ifdef _DEBUG
		NamedPipeManager::instance().writeMessage(formattedArgs, false, true);
#endif
	}

	void logError(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

		NamedPipeManager::instance().writeMessage(formattedArgs, true, false);
		writeToLogFile(formattedArgs, "ERROR");
	}

	void logLine(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		string format(fmt);
		format += '\n';
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

		NamedPipeManager::instance().writeMessage(formattedArgs);
		writeToLogFile(formattedArgs, "INFO");
	}
}