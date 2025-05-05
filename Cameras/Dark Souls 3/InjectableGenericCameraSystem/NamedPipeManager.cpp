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


	void NamedPipeManager::writeTextPayload(const std::string& messageText, MessageType typeOfMessage) const
	{
		if (!_dllToClientPipeConnected)
		{
			return;
		}

		uint8_t payload[IGCS_MAX_MESSAGE_SIZE];
		payload[0] = uint8_t(typeOfMessage);
		strncpy_s((char*)(&payload[1]), IGCS_MAX_MESSAGE_SIZE-2, messageText.c_str(), messageText.length());
		DWORD numberOfBytesWritten;
		WriteFile(_dllToClientPipe, payload, static_cast<DWORD>(messageText.length() + 1), &numberOfBytesWritten, nullptr);
	}

	void NamedPipeManager::writeBinaryPayload(size_t value, MessageType typeOfMessage) const
	{
		if (!_dllToClientPipeConnected)
		{
			return;
		}

		// Create a buffer with room for the message type byte plus the data
		uint8_t payload[sizeof(value) + 1];

		payload[0] = static_cast<uint8_t>(typeOfMessage);

		// Copy the binary representation of the size_t value into the payload buffer starting at index 1.
		memcpy(&payload[1], &value, sizeof(size_t));

		DWORD numberOfBytesWritten;
		// Write the total size: 1 byte for the type + sizeof(size_t)
		WriteFile(_dllToClientPipe, payload, 1 + sizeof(size_t), &numberOfBytesWritten, nullptr);
		MessageHandler::logDebug("NamedPipeManager::writeFloatPayload::Number of bytes written: %u", numberOfBytesWritten);
	}

	void NamedPipeManager::writeBinaryPayload(const void* data, size_t dataSize, MessageType typeOfMessage, bool logDebug)
	{
		if (!_dllToClientPipeConnected)
		{
			return;
		}

		// Check if the data can fit in our message buffer
		if (dataSize + 1 > IGCS_MAX_MESSAGE_SIZE)
		{
			MessageHandler::logError("Binary payload too large to send (size: %zu bytes)", dataSize);
			return;
		}

		// Create a buffer with room for the message type byte plus the data
		std::vector<uint8_t> payload(dataSize + 1);

		// Set the message type as the first byte
		payload[0] = static_cast<uint8_t>(typeOfMessage);

		// Copy the data after the message type byte
		memcpy(payload.data() + 1, data, dataSize);

		// Write the data to the pipe
		DWORD numberOfBytesWritten;
		BOOL writeSuccess = WriteFile(
			_dllToClientPipe,           // Pipe handle
			payload.data(),             // Buffer to write from
			static_cast<DWORD>(payload.size()), // Number of bytes to write
			&numberOfBytesWritten,      // Number of bytes written
			nullptr);                   // Not overlapped

		if (logDebug)
		{
			if (!writeSuccess || numberOfBytesWritten != payload.size())
			{
				MessageHandler::logError("Failed to write binary payload to pipe. Error: %u", GetLastError());
			}
			else
			{
				MessageHandler::logDebug("Successfully wrote %u bytes of binary data to pipe", numberOfBytesWritten);
			}
		}

	}

	void NamedPipeManager::writeFloatPayload(float value, MessageType typeOfMessage) const
	{
		if (!_dllToClientPipeConnected)
		{
			return;
		}

		// Create a buffer with room for the message type byte plus the float value
		uint8_t payload[sizeof(float) + 1];

		payload[0] = static_cast<uint8_t>(typeOfMessage);

		// Copy the float value into the payload buffer
		memcpy(&payload[1], &value, sizeof(float));

		// Debug logging
		//MessageHandler::logDebug("Sending binary payload: type=%d, value=%.4f",
			//static_cast<int>(typeOfMessage), value);

		DWORD numberOfBytesWritten;
		WriteFile(_dllToClientPipe, payload, sizeof(payload), &numberOfBytesWritten, nullptr);

		if (numberOfBytesWritten != sizeof(payload))
		{
			MessageHandler::logError("Failed to write complete binary payload. Written: %u, Expected: %zu",
				numberOfBytesWritten, sizeof(payload));
		}
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
				uint8_t buffer[14*1024];
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
		case MessageType::CameraPathData:
			handlePathAction(buffer, bytesRead);
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
		//cases for hotsampling
		case ActionMessageType::setWidth:
			Globals::instance().handleActionPayload(buffer, bytesRead);
			break;
		case ActionMessageType::setHeight:
			Globals::instance().handleActionPayload(buffer, bytesRead);
			break;
		case ActionMessageType::setResolution:
			//call functions to set the resolution
			//GameSpecific::CameraManipulator::displayResolution(Globals::instance().settings().hsWidth, Globals::instance().settings().hsHeight);
			//GameSpecific::CameraManipulator::setResolution(Globals::instance().settings().hsWidth, Globals::instance().settings().hsHeight);
			break;
		default:
			// ignore the rest
			break;
		}
	}

	void NamedPipeManager::handlePathAction(uint8_t buffer[], DWORD bytesRead)
	{
		if (bytesRead < 2 || buffer[0] != uint8_t(MessageType::CameraPathData))
		{
			return;
		}
		//CameraPathManager& currPM = CameraPathManager::instance();
		switch (static_cast<PathActionType>(buffer[1]))
		{
		case PathActionType::addPath:
			//MessageHandler::logDebug("New Path Message Received");
			CameraPathManager::instance().handleAddPathMessage(buffer, bytesRead);
			break;
		case PathActionType::deletePath:
			//MessageHandler::logDebug("NamedPipeSubsystem::deletePath Case::Delete Path Message Received");
			CameraPathManager::instance().handleDeletePathMessage();
			break;
		case PathActionType::addNode:
			//MessageHandler::logDebug("NamedPipeSubsystem::addNode Case::Add Node Message Received");
			CameraPathManager::instance().handleAddNodeMessage();
			break;
		case PathActionType::insertNode:
			//MessageHandler::logDebug("NamedPipeSubsystem::insertNode Case::Insert Node Message Received");
			CameraPathManager::instance().handleInsertNodeBeforeMessage(buffer, bytesRead);
			break;
		case PathActionType::playPath:
			//MessageHandler::logDebug("NamedPipeSubsystem::playPath Case::playPath Message Received");
			CameraPathManager::instance().handlePlayPathMessage();
			break;
		case PathActionType::gotoNode:
			//MessageHandler::logDebug("NamedPipeSubsystem::gotoNodeIndex Case::gotoNodeIndex Message Received");
			CameraPathManager::instance().goToNodeSetup(buffer, bytesRead);
			break;
		case PathActionType::deleteNode:
			//MessageHandler::logDebug("NamedPipeSubsystem::deleteNode Case::Delete Node Message Received");
			CameraPathManager::instance().handleDeleteNodeMessage(buffer, bytesRead);
			break;
		case PathActionType::stopPath:
			//MessageHandler::logDebug("NamedPipeSubsystem::stopPath Case::Refresh Path Message Received");
			CameraPathManager::instance().handleStopPathMessage();
			break;
		case PathActionType::refreshPath:
			//MessageHandler::logDebug("NamedPipeSubsystem::refreshPath Case::Refresh Path Message Received");
			CameraPathManager::instance().refreshPath();
			break;
		case PathActionType::updateNode:
			//MessageHandler::logDebug("NamedPipeSubsystem::updateNode Case::Update Node Message Received");
			CameraPathManager::instance().updateNode(buffer, bytesRead);
			break;
		case PathActionType::selectedPathUpdate:
			//MessageHandler::logDebug("NamedPipeSubsystem::selectedPathUpdate Case::Selected Path Update Message Received");
			CameraPathManager::instance().setSelectedPath(buffer, bytesRead);
			break;
		case PathActionType::selectedNodeIndexUpdate:
			//MessageHandler::logDebug("NamedPipeSubsystem::selectedNodeIndex Case::Selected NodeIndex Update Message Received");
			CameraPathManager::instance().setSelectedNodeIndex(buffer, bytesRead);
			break;
		case PathActionType::pathManagerStatus:
			//MessageHandler::logDebug("NamedPipeSubsystem::pathManagerStatus Case::Path Manager Update Message Received");
			CameraPathManager::instance().setPathManagerStatus(buffer, bytesRead);
			break;
		case PathActionType::pathScrubbing:
			//MessageHandler::logDebug("NamedPipeSubsystem::pathScrubbing Case::Path Scrubbing Message Received");
			CameraPathManager::instance().handlePathScrubbingMessage(buffer, bytesRead);
			break;
		default:
			// ignore the rest
			break;
		}
	}
}
