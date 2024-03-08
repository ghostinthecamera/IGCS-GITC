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
#pragma once
#include "stdafx.h"
#include <string>
#include "Defaults.h"

namespace IGCS
{
	class NamedPipeManager
	{
	public:
		NamedPipeManager();
		~NamedPipeManager();

		static NamedPipeManager& instance();

		void connectDllToClient();
		void startListening();
		void writeTextPayload(const std::string& messageText, MessageType typeOfMessage);
		void writeMessage(const std::string& messageText);
		void writeMessage(const std::string& messageText, bool isError);
		void writeMessage(const std::string& messageText, bool isError, bool isDebug);
		void writeNotification(const std::string& notificationText);
		DWORD listenerThread();

	private:
		void handleMessage(uint8_t buffer[], DWORD bytesRead);
		void handleAction(uint8_t buffer[], DWORD bytesRead);

		HANDLE _dllToClientPipe;
		HANDLE _clientToDllPipe;
		bool _dllToClientPipeConnected;
		bool _clientToDllPipeConnected;
	};
}

