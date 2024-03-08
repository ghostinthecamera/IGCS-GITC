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

namespace IGCS::MessageHandler
{
	
	void addNotification(const string& notificationText)
	{
		NamedPipeManager::instance().writeNotification(notificationText);
	}


	void logDebug(const char* fmt, ...)
	{
#ifdef _DEBUG
		va_list args;
		va_start(args, fmt);
		const string formattedArgs = Utils::formatStringVa(fmt, args);
		va_end(args);

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
	}
}
