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
#include "NamedPipeManager.h"
#include "Defaults.h"
#include "Console.h"
#include <string>
#include "Globals.h"
#include "InputHooker.h"
#include "CameraManipulator.h"

namespace IGCS
{
	// Workaround for having a method as the actual thread func. See: https://stackoverflow.com/a/1372989
	static DWORD WINAPI staticListenerThread(LPVOID lpParam)
	{
		// our message handling will be very fast so we'll do this on the listener thread, no need to offload that to yet another thread yet.
		auto This = (NamedPipeManager*)lpParam;
		return This->listenerThread();
	}

	NamedPipeManager::NamedPipeManager(): _clientToDllPipe(nullptr), _clientToDllPipeConnected(false), _dllToClientPipe(nullptr), _dllToClientPipeConnected(false)
	{
	}

	
	NamedPipeManager::~NamedPipeManager()
	= default;


	NamedPipeManager& NamedPipeManager::instance()
	{
		static NamedPipeManager theInstance;
		return theInstance;
	}

	
	void NamedPipeManager::connectDllToClient()
	{
		if(_dllToClientPipeConnected)
		{
			return;
		}
		_dllToClientPipe = CreateFile(TEXT(IGCS_PIPENAME_DLL_TO_CLIENT), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		_dllToClientPipeConnected = (_dllToClientPipe != INVALID_HANDLE_VALUE);
		if(!_dllToClientPipeConnected)
		{
			Console::WriteError("Couldn't connect to named pipe DLL -> Client. Please start the client first.");
		}
	}

	
	void NamedPipeManager::startListening()
	{
		// create a thread to listen to the named pipe for messages and handle them.
		DWORD threadID;
		HANDLE threadHandle = CreateThread(nullptr, 0, staticListenerThread, (LPVOID)this, 0, &threadID);
	}


	void NamedPipeManager::writeTextPayload(const std::string& messageText, MessageType typeOfMessage)
	{
		if (!_dllToClientPipeConnected)
		{
			return;
		}

		uint8_t payload[IGCS_MAX_MESSAGE_SIZE];
		payload[0] = uint8_t(typeOfMessage);
		strncpy_s((char*)(&payload[1]), IGCS_MAX_MESSAGE_SIZE-2, messageText.c_str(), messageText.length());
		DWORD numberOfBytesWritten;
		WriteFile(_dllToClientPipe, payload, messageText.length() + 1, &numberOfBytesWritten, nullptr);
	}

	
	void NamedPipeManager::writeMessage(const std::string& messageText)
	{
		writeMessage(messageText, false, false);
	}

	
	void NamedPipeManager::writeMessage(const std::string& messageText, bool isError)
	{
		writeMessage(messageText, isError, false);
	}

	
	void NamedPipeManager::writeMessage(const std::string& messageText, bool isError, bool isDebug)
	{
		MessageType typeOfMessage = MessageType::NormalTextMessage;
		if(isError)
		{
			typeOfMessage = MessageType::ErrorTextMessage;
		}
		else
		{
			if(isDebug)
			{
				typeOfMessage = MessageType::DebugTextMessage;
			}
		}
		writeTextPayload(messageText, typeOfMessage);
	}

	
	void NamedPipeManager::writeNotification(const std::string& notificationText)
	{
		writeTextPayload(notificationText, MessageType::Notification);
	}


	DWORD NamedPipeManager::listenerThread()
	{
		// Set security ACLs as by default the connecting party has to be admin.
		SECURITY_ATTRIBUTES sa;
		sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
		bool saInitFailed = false;
		if (!InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
		{
			saInitFailed = true;
		}
		if (sa.lpSecurityDescriptor != nullptr)
		{
			// Grant everyone access, otherwise we are required to run the client as admin. We likely will need to do that anyway, but to avoid
			// the requirement in general we allow everyone. 
			if (!SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, nullptr, FALSE))
			{
				saInitFailed = true;
			}
			sa.nLength = sizeof sa;
			sa.bInheritHandle = TRUE;
		}
		else
		{
			saInitFailed = true;
		}
		_clientToDllPipe = CreateNamedPipe(TEXT(IGCS_PIPENAME_CLIENT_TO_DLL), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 0, IGCS_MAX_MESSAGE_SIZE,
											NMPWAIT_WAIT_FOREVER, saInitFailed ? nullptr : &sa);
		_clientToDllPipeConnected = (_clientToDllPipe != INVALID_HANDLE_VALUE);
		if (!_clientToDllPipeConnected)
		{
			Console::WriteError("Couldn't create the Client -> DLL named pipe.");
			return 1;
		}
		while (_clientToDllPipe != INVALID_HANDLE_VALUE)
		{
			auto connectResult = ConnectNamedPipe(_clientToDllPipe, nullptr);
			if(connectResult!=0 || GetLastError()==ERROR_PIPE_CONNECTED)
			{
				uint8_t buffer[1024];
				DWORD bytesRead;
				while (ReadFile(_clientToDllPipe, buffer, sizeof(buffer), &bytesRead, nullptr))
				{
					handleMessage(buffer, bytesRead);
				}
			}
		}
		return 0;
	}

	
	void NamedPipeManager::handleMessage(uint8_t buffer[], DWORD bytesRead)
	{
		if(bytesRead<2)
		{
			//not useful
			return;
		}
		switch(static_cast<MessageType>(buffer[0]))
		{
		case MessageType::Setting:
			Globals::instance().handleSettingMessage(buffer, bytesRead);
			GameSpecific::CameraManipulator::applySettingsToGameState();
			break;
		case MessageType::KeyBinding:
			Globals::instance().handleKeybindingMessage(buffer, bytesRead);
			break;
		case MessageType::Action:
			handleAction(buffer, bytesRead);
			break;
		default:
			// ignore the rest
			break;
		}
	}


	void NamedPipeManager::handleAction(uint8_t buffer[], DWORD bytesRead)
	{
		if(bytesRead<2 || buffer[0] != uint8_t(MessageType::Action))
		{
			return;
		}
		switch(static_cast<ActionMessageType>(buffer[1]))
		{
		case ActionMessageType::RehookXInput:
			InputHooker::setXInputHook(true);
			break;
		}
	}
}
