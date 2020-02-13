//////////////////////////////////////////////////////////////////////
// By Adam Dobos: https://github.com/adam10603/XInput-wrapper
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Gamepad.h"
#include <Windows.h>
#include <Xinput.h>
#include <algorithm>
#include <limits>

#define clamp(v, _min, _max) max(min(v, _max), _min)

XINPUT_STATE* Gamepad::getState() { return &gpState; }
int Gamepad::getIndex() { return gpIndex; }
bool Gamepad::isConnected() { return connected; }
void Gamepad::setButtonDownCallback(function<void(button_t)> fn) { buttonDownCallback = fn; }
void Gamepad::setButtonUpCallback(function<void(button_t)> fn) { buttonUpCallback = fn; }
bool Gamepad::isButtonPressed(button_t b) { return (buttonState & b); }
void Gamepad::setInvertLStickY(bool b) { invertLSY = b; }
void Gamepad::setInvertRStickY(bool b) { invertRSY = b; }

void Gamepad::update() {
	ZeroMemory(&gpState, sizeof(XINPUT_STATE));
	connected = (XInputGetState(gpIndex, &gpState) == ERROR_SUCCESS);
	if (buttonState != gpState.Gamepad.wButtons) {
		WORD stateDiff = buttonState ^ gpState.Gamepad.wButtons;
		WORD buttonStateCopy = buttonState;
		for (WORD i{ 0x1 };; i <<= 0x1) {
			if (stateDiff & 0x1) {
				if (buttonStateCopy & 0x1) {
					if (buttonUpCallback)
						buttonUpCallback(static_cast<button_t>(i));
				} else {
					if (buttonDownCallback)
						buttonDownCallback(static_cast<button_t>(i));
				}
			}
			if (i == 0x8000) break;
			else {
				stateDiff >>= 0x1;
				buttonStateCopy >>= 0x1;
			}
		}
	}
	buttonState = gpState.Gamepad.wButtons;
}

vec2 Gamepad::getLStickPosition() {
	auto x = gpState.Gamepad.sThumbLX;
	auto y = gpState.Gamepad.sThumbLY;
	if (abs(x) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
		abs(y) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) {
		return { 0.0f, 0.0f };
	} else {
		auto xf = float{ static_cast<float>(x) / static_cast<float>(SHRT_MAX) };
		auto yf = float{ (invertLSY ? -1.0f : 1.0f) * static_cast<float>(y) / static_cast<float>(SHRT_MAX) };
		return { clamp(xf, -1.0f, 1.0f), clamp(yf, -1.0f, 1.0f) };
	};
}

vec2 Gamepad::getRStickPosition() {
	auto x = gpState.Gamepad.sThumbRX;
	auto y = gpState.Gamepad.sThumbRY;
	if (abs(x) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
		abs(y) <= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) {
		return { 0.0f, 0.0f };
	} else {
		auto xf = float{ static_cast<float>(x) / static_cast<float>(SHRT_MAX) };
		auto yf = float{ (invertRSY ? -1.0f : 1.0f) * static_cast<float>(y) / static_cast<float>(SHRT_MAX) };
		return { clamp(xf, -1.0f, 1.0f), clamp(yf, -1.0f, 1.0f) };
	};
}

float Gamepad::getLTrigger() {
	auto x = gpState.Gamepad.bLeftTrigger;
	return (x > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? static_cast<float>(x) / 255.0f : 0.0f;
}

float Gamepad::getRTrigger() {
	auto x = gpState.Gamepad.bRightTrigger;
	return (x > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? static_cast<float>(x) / 255.0f : 0.0f;
}

void Gamepad::vibrate(float L, float R) {
	L = clamp(L, 0.0f, 1.0f);
	R = clamp(R, 0.0f, 1.0f);
	XINPUT_VIBRATION vState;
	ZeroMemory(&vState, sizeof(XINPUT_VIBRATION));
	int iL = static_cast<WORD>(L * 65535.0f + 0.4999f);
	int iR = static_cast<WORD>(R * 65535.0f + 0.4999f);
	vState.wLeftMotorSpeed = iL;
	vState.wRightMotorSpeed = iR;
	XInputSetState(gpIndex, &vState);
}
