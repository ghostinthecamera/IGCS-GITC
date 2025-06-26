////////////////////////////////////////////////////////////////////////////////////////////////////////
// Part of Injectable Generic Camera System
// Copyright(c) 2019, Frans Bouma
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
#include <filesystem>
#include "MessageHandler.h"
#include "GameConstants.h"

namespace IGCS
{
	// forward declaration to avoid cyclic dependency.
	class AOBBlock;
}

namespace IGCS::Utils
{
	static constexpr auto setIfChanged = []<typename T0>(T0 & dst, auto const& src) {
		using DstT = std::decay_t<T0>;
		DstT newVal = static_cast<DstT>(src);
		if (dst != newVal) {
			dst = newVal;
		}
	};

	//Checks an incoming value against an existing value and then 1) calls the designated function and
	//can pass the new value to that function and 2 assigns the value of the incoming variable to the destination
	static constexpr auto callOnChange = []<typename D, typename S, typename Fn>(
		D& destination,
		S const& source,
		Fn&& functiontocall) {
		using Decayed = std::decay_t<D>;
		Decayed newVal = static_cast<Decayed>(source);
		if (destination != newVal) {
			std::invoke(std::forward<Fn>(functiontocall), newVal);
			destination = newVal;
		}
	};

	struct handle_data {
		unsigned long process_id;
		HWND best_handle;
	};

	template <typename T>
	T clamp(T value, T min, T max, T defaultValue)
	{
		return value < min ? defaultValue
			: value > max ? defaultValue : value;
	}

	template <typename T>
	T clamp(T value, T min, T defaultValue)
	{
		return value < min ? defaultValue : value;
	}

	template <typename T>
	T rangeClamp(T value, T min, T max)
	{
		return (value < min) ? min : (value > max) ? max : value;
	}

	template <typename T>
	constexpr const T& Clamp(const T& value, const T& low, const T& high) 
	{
		return (value < low) ? low : ((high < value) ? high : value);
	}

	// Variadic printOnce function
	template <typename... Args>
	void printOnce(const char* fmt, Args... args) 
	{
		static bool hasPrinted = false; // Static variable to track if the message has been printed
		if (!hasPrinted) {
			MessageHandler::logDebug(fmt, args...); // Forward the format string and arguments to logDebug
			hasPrinted = true; // Ensure it doesn't print again
		}
	}

	enum class EulerOrder {
		XYZ,  // corresponds to: i = 2, j = 1, k = 0
		XZY,  // i = 1, j = 2, k = 0
		YXZ,  // i = 2, j = 0, k = 1
		YZX,  // i = 0, j = 2, k = 1
		ZXY,  // i = 1, j = 0, k = 2
		ZYX   // i = 0, j = 1, k = 2
	};

	//---------------------------------------------------------------------
	// Structure holding Euler angles in double precision.
	// The returned values represent (pitch, yaw, roll) in radians.
	//---------------------------------------------------------------------
	struct EulerAnglesDouble {
		double pitch;
		double yaw;
		double roll;
	};

	HWND findMainWindow(unsigned long process_id);
	MODULEINFO getModuleInfoOfContainingProcess();
	MODULEINFO getModuleInfoOfDll(LPCWSTR libraryName);
	bool findAOBPattern(LPBYTE imageAddress, DWORD imageSize, AOBBlock* const toScanFor);
	uint8_t CharToByte(char c);
	LPBYTE calculateAbsoluteAddress(AOBBlock* locationData, int nextOpCodeOffset);
	std::string formatString(const char* fmt, ...);
	std::string formatStringVa(const char* fmt, va_list args);
	bool stringStartsWith(const char *a, const char *b);
	bool keyDown(int virtualKeyCode);
	bool altPressed();
	bool ctrlPressed();
	bool shiftPressed();
	std::string vkCodeToString(int vkCode);
	float floatFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	int intFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	double doubleFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	std::string stringFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
	std::filesystem::path obtainHostExeAndPath();
	void toggleNOPState(AOBBlock& hookData, int numberOfBytes, bool enabled);
	void saveBytesWrite(AOBBlock& hookData, int numberOfBytes, uint8_t* bytestoWrite, bool enabled);
	DirectX::XMFLOAT3 QuaternionToEulerAngles(DirectX::XMVECTOR q, EulerOrder order = MULTIPLICATION_ORDER);
	inline float clampAngle(float angle);
	DirectX::XMVECTOR generateEulerQuaternionManual(const DirectX::XMFLOAT3& euler, EulerOrder order, bool negatePitch = false, bool negateYaw = false, bool negateRoll = false);
	//DirectX::XMVECTOR generateEulerQuaternion(const DirectX::XMFLOAT3& euler, EulerOrder order, bool negatePitch = false, bool negateYaw = false, bool negateRoll = false);
	DirectX::XMVECTOR generateEulerQuaternion(const DirectX::XMFLOAT3& euler, EulerOrder order = MULTIPLICATION_ORDER, bool negatePitch = NEGATE_PITCH, bool negateYaw = NEGATE_YAW, bool negateRoll = NEGATE_ROLL);
	DirectX::XMFLOAT3 RotationMatrixToEulerAngles(const DirectX::XMMATRIX& m, EulerOrder order, bool isColumnMajor = false, bool isViewMatrix = false);
	DirectX::XMMATRIX EulerAnglesToRotationMatrix(const DirectX::XMFLOAT3& euler, EulerOrder order,
		bool negatePitch = false,
		bool negateYaw = false,
		bool negateRoll = false);
	DirectX::XMFLOAT3 ExtractCameraPosition(const DirectX::XMMATRIX& m, bool isColumnMajor = false, bool isViewMatrix = true);
	DirectX::XMMATRIX CreateViewMatrixFromEuler(const DirectX::XMFLOAT3& cameraPos, const DirectX::XMFLOAT3& euler,
		EulerOrder order,
		bool negatePitch = false,
		bool negateYaw = false,
		bool negateRoll = false);
	double getCurrentTimeSeconds();
	DirectX::XMMATRIX CreateViewMatrix(const DirectX::XMFLOAT3& cameraPos, const DirectX::XMVECTOR& orientation);
	const char* getExecutableName();
	const wchar_t* getExecutableNameW();
	uint8_t uintFromBytes(uint8_t byteArray[], DWORD arrayLength, int startIndex);
}