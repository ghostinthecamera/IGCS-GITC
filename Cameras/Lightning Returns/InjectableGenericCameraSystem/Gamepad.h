//////////////////////////////////////////////////////////////////////
// By Adam Dobos: https://github.com/adam10603/XInput-wrapper
//////////////////////////////////////////////////////////////////////
#pragma once

#if defined( _M_X64 )
#define _AMD64_
#elif defined ( _M_IX86 )
#define _X86_
#elif defined( _M_ARM )
#define _ARM_
#endif

#include <functional>
#include <Xinput.h>
using namespace std;

/* The positions of the analog sticks will be returned as 'vec2'.
 * If you have your own type similar to 'vec2', feel free
 * to modify the code to use that. As long as it has '.x' and '.y'
 * floating point components and a simple constructor that
 * takes values for them, it should be a drop-in replacement.
 * In case you don't have such a type, here's one for ya:
 */
struct vec2 {
	vec2(float _x, float _y) : x{ _x }, y{ _y } {}
	float x = 0.0f;
	float y = 0.0f;
};

class Gamepad {
public:
	// These values represent the buttons on the gamepad.
	enum button_t {
		A = XINPUT_GAMEPAD_A,
		B = XINPUT_GAMEPAD_B,
		X = XINPUT_GAMEPAD_X,
		Y = XINPUT_GAMEPAD_Y,
		UP = XINPUT_GAMEPAD_DPAD_UP,
		DOWN = XINPUT_GAMEPAD_DPAD_DOWN,
		LEFT = XINPUT_GAMEPAD_DPAD_LEFT,
		RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
		LB = XINPUT_GAMEPAD_LEFT_SHOULDER,
		RB = XINPUT_GAMEPAD_RIGHT_SHOULDER,
		LSTICK = XINPUT_GAMEPAD_LEFT_THUMB,
		RSTICK = XINPUT_GAMEPAD_RIGHT_THUMB,
		START = XINPUT_GAMEPAD_START,
		BACK = XINPUT_GAMEPAD_BACK
	};
	// If you have multiple gamepads, create multiple instances of the class using different indexes. The connected gamepads are numbered 0-4. The default argument is 0, meaning this instance will take control of the first (or only one) gamepad connected to the system.
	Gamepad(int index = 0) : gpIndex{ index }, buttonDownCallback{ nullptr }, buttonUpCallback{ nullptr }, buttonState{ 0x0 } {}

	// Set a function (of type 'void', with a 'button_t' argument) to be called each time a button is pressed on the gamepad. The value of the argument should be checked against values of the 'button_t' enum to determine which button was pressed.
	void setButtonDownCallback(function<void(button_t)> fn);
	// Set a function (of type 'void', with a 'button_t' argument) to be called each time a button is released on the gamepad. The value of the argument should be checked against values of the 'button_t' enum to determine which button was released.
	void setButtonUpCallback(function<void(button_t)> fn);
	// Ths update function should be called every cycle of your app (before any other member calls) to keep things up to date. NOTE: All data that other member functions return is actually read from the device at the time you call this function, and will not be updated until you call 'update()' again.
	void update();
	// Indicates if the gamepad is connected or not. This should be checked before fetching any data.
	bool isConnected();
	// Indicates if a button (specified by the argument) is currently pressed or not (well, at the time of the last 'update()' call anyway).
	bool isButtonPressed(button_t b);
	// Returns the position of the left analog stick. '.x' and '.y' range from -1.0 to 1.0.
	vec2 getLStickPosition();
	// Returns the position of the right analog stick. '.x' and '.y' range from -1.0 to 1.0.
	vec2 getRStickPosition();
	// Returns the position of the left trigger. Range is 0.0 (not pressed) to 1.0 (pressed all the way).
	float getLTrigger();
	// Returns the position of the right trigger. Range is 0.0 (not pressed) to 1.0 (pressed all the way).
	float getRTrigger();
	// Vibrates the gamepad. 'L' and 'R' define the vibration intensity of the left and right motors. They should be in the 0.0 to 1.0 range. NOTE: to turn vibrations off, you must call this function again with both arguments being 0.0.
	void vibrate(float L, float R);
	// If set to true, the Y-axis of the left analog stick will be inverted.
	void setInvertLStickY(bool b);
	// If set to true, the Y-axis of the right analog stick will be inverted.
	void setInvertRStickY(bool b);

	// Only use this if you want to use some XInput functionality directly. It returns a pointer to the current state.
	XINPUT_STATE* getState();
	// Returns the gamepad's index (the argument the constructor was given). Kind of pointless, but whatever.
	int getIndex();
private:
	function<void(button_t)> buttonDownCallback;
	function<void(button_t)> buttonUpCallback;
	XINPUT_STATE gpState;
	int gpIndex;
	WORD buttonState;
	bool connected;
	bool invertLSY;
	bool invertRSY;
};
