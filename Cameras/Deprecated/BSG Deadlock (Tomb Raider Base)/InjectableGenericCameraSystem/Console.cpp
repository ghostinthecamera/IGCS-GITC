////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2016, Frans Bouma
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
#include "Console.h"

using namespace std;

Console::Console()
{
}

Console::~Console()
{
}

void Console::Release()
{
	FreeConsole();
}

void Console::WriteHeader()
{
	SetColor(CONSOLE_WHITE);
	cout << "Injectable camera tools for " << GAME_NAME << ". Version: " << CAMERA_VERSION << endl;
	WriteLine("Powered by Injectable Generic Camera System by Otis_Inf");
	WriteLine("Get your copy at: https://github.com/FransBouma/InjectableGenericCameraSystem");
	cout << "Camera credits: " << CAMERA_CREDITS << endl;
	SetColor(9);
	WriteLine("-----------------------------------------------------------------------------");
	SetColor(CONSOLE_NORMAL);
}

void Console::WriteLine(const string& toWrite, int color)
{
	SetColor(color);
	WriteLine(toWrite);
	SetColor(CONSOLE_NORMAL);
}


void Console::WriteLine(const string& toWrite)
{
	cout << toWrite << endl;
}


void Console::WriteError(const string& error)
{
	cerr << error << endl;
}


void Console::Init()
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());

	// Redirect the CRT standard input, output, and error handles to the console
	FILE *fp;
	freopen_s(&fp, "CONIN$", "r", stdin);
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);

	//Clear the error state for each of the C++ standard stream objects. 
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();

	SetColor(15);
	SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 12);
}


void Console::SetColor(int color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}