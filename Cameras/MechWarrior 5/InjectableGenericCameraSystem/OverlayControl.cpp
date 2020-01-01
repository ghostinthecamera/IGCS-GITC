#include "stdafx.h"
#include "OverlayControl.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "OverlayConsole.h"
#include "GameConstants.h"
#include <algorithm>
#include <mutex>
#include "Defaults.h"
#include "Globals.h"
#include "CameraManipulator.h"
#include "Input.h"
#include <atomic>
#include "InputHooker.h"
#include "Console.h"

using namespace std;

namespace IGCS::OverlayControl
{
	//-----------------------------------------------
	// private structs
	struct Notification
	{
		string notificationText;
		float timeFirstDisplayed;		// if < 0, never displayed
	};

	//-----------------------------------------------
	// statics
	static vector<Notification> _activeNotifications;
	static CRITICAL_SECTION _notificationCriticalSection;
	static atomic_bool _displaySplash = true;
	static atomic<float> _timeSplashFirstShown = -1;
	static ImVec2 _splashWindowSize;
	static atomic_int _menuItemSelected = 0;
	static atomic_short _actionKeyBindingEditing = -1;
	static bool _showMainWindow = false;
	
	//-----------------------------------------------
	// forward declarations
	void renderNotifications();
	void renderAbout();
	void renderHelp();
	void renderSettings();
	void renderMainWindow();
	void renderKeyBindings();
	void renderSplash();
	void updateNotificationStore();
	void showHelpMarker(const char* desc);
	void startKeyBindingCapturing(short actionType);
	void endKeyBindingCapturing(bool acceptCollectedBinding);
	void initImGuiStyle();

	//-----------------------------------------------
	// code

	void init()
	{
		InitializeCriticalSectionAndSpinCount(&_notificationCriticalSection, 0x400);
	}

	void addNotification(string notificationText)
	{
		EnterCriticalSection(&_notificationCriticalSection);
			Notification toAdd;
			toAdd.notificationText = notificationText;
			toAdd.timeFirstDisplayed = -1.0f;
			_activeNotifications.push_back(toAdd);
			OverlayConsole::instance().logLine(notificationText.c_str());
		LeaveCriticalSection(&_notificationCriticalSection);
	}

	void renderOverlay()
	{
		// set io values FIRST, as NewFrame will reset IO values otherwise and it looks at the values for mousewheel for sizing in... new frame!
		Input::setKeyboardMouseStateInImGuiIO();
		Globals::instance().saveSettingsIfRequired(ImGui::GetIO().DeltaTime);

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		auto& io = ImGui::GetIO();
		io.MouseDrawCursor = _showMainWindow;
		renderSplash();
		renderMainWindow();
		renderNotifications();
		ImGui::Render();
	}


	void renderMainWindow()
	{
		if (!_showMainWindow)
		{
			return;
		}
		ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_FirstUseEver);
		string title = string("Injectable Camera Tools v") + CAMERA_VERSION + " for " + GAME_NAME;
		ImGui::Begin(title.c_str(), &_showMainWindow, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginMenuBar())
		{
			auto itemSpacing = ImVec2(ImGui::GetStyle().ItemSpacing.x*2.0f, ImGui::GetStyle().ItemSpacing.y);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, itemSpacing);
			const char *const menu_items[] = { "Settings", "Key-bindings", "Console", "Help", "About" };
			for (int i = 0; i < 5; i++)
			{
				if (ImGui::Selectable(menu_items[i], _menuItemSelected == i, 0, ImVec2(ImGui::CalcTextSize(menu_items[i]).x, 0)))
				{
					_menuItemSelected = i;
				}
				ImGui::SameLine();
			}
			ImGui::PopStyleVar();
			ImGui::EndMenuBar();
		}

		ImGui::Spacing();

		switch (_menuItemSelected)
		{
		case 0: // settings
			renderSettings();
			break;
		case 1: // key bindings
			renderKeyBindings();
			break;
		case 2: // console
			IGCS::OverlayConsole::instance().draw();
			break;
		case 3:	// help
			renderHelp();
			break;
		case 4:	// about
			renderAbout();
			break;
		}
		ImGui::End();
	}


	void renderAbout()
	{
		ImGui::PushTextWrapPos();
		ImGui::Text("Injectable Camera Tools v%s for %s", CAMERA_VERSION, GAME_NAME);
		ImGui::Text("Camera credits: %s", CAMERA_CREDITS);
		ImGui::Spacing();
		ImGui::Text(R"(Powered by Injectable Generic Camera System (IGCS). 
Copyright (c) 2019 Otis_Inf (Frans Bouma). All rights reserved.
https://github.com/FransBouma/InjectableGenericCameraSystem

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.)");
		if (ImGui::CollapsingHeader("Dear ImGui"))
		{
			ImGui::TextUnformatted(R"(Copyright (c) 2014-2015 Omar Cornut and ImGui contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.)");
		}
		if (ImGui::CollapsingHeader("MinHook"))
		{
			ImGui::TextUnformatted(R"(Copyright (C) 2009-2017 Tsuda Kageyu. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.)");
		}
		if (ImGui::CollapsingHeader("Miscellaneous"))
		{
			ImGui::TextUnformatted(R"(Uses CDataFile by Gary McNickle for ini file processing.

Special thanks to: 
* Crosire, for creating and opening the Reshade sourcecode. It was a great help with ImGui.
* HattiWatti, for tips and small utility code.
* The users of my camera's, you all are a great inspiration to make the best camera tools possible!)");
		}
		ImGui::PopTextWrapPos();
	}


	void renderHelp()
	{
		if (ImGui::CollapsingHeader("General info"))
		{
			ImGui::PushTextWrapPos();
			ImGui::TextUnformatted("* When the main window is open, all input of keyboard / mouse to the game is blocked and the camera is locked.");
			ImGui::TextUnformatted("* All changes you make to the main window (position/size) are saved to a file in the game root folder.");
			ImGui::TextUnformatted("* Most settings you change will make the settings to be saved to a file in the game root folder.");
			ImGui::TextUnformatted("* When you block input to the game, the device(s) which control the camera are blocked, all other devices are still sending input to the game.");
			ImGui::TextUnformatted("* You'll get a notification in the top left corner when you change something with the keyboard, like enable/disable the camera.");
			ImGui::TextUnformatted("* The resolution scale factor is reset to the game menu value when the camera is disabled");
			ImGui::PopTextWrapPos();
		}

		if (ImGui::CollapsingHeader("Camera tools controls"))
		{
			ImGui::Text("Show / Hide Camera tools main window : %s", Globals::instance().getActionData(ActionType::ToggleOverlay)->toString().c_str());
			ImGui::Text("Resize font                          : Ctrl + Mouse wheel");
			ImGui::Text("Enable/Disable camera                : %s", Globals::instance().getActionData(ActionType::CameraEnable)->toString().c_str());
			ImGui::Text("Lock / unlock camera movement        : %s", Globals::instance().getActionData(ActionType::CameraLock)->toString().c_str());
			ImGui::Text("Toggle HUD                           : %s", Globals::instance().getActionData(ActionType::HudToggle)->toString().c_str());
			ImGui::Text("Faster rotate / move                 : Alt + rotate / move");
			ImGui::Text("Slower rotate / move                 : Ctrl + rotate / move");
			ImGui::Text("Faster rotate / move                 : Controller Y-button + l/r-stick");
			ImGui::Text("Slower rotate / move                 : Controller X-button + l/r-stick");
			ImGui::Text("Rotate camera up/down                : %s/%s or mouse or r-stick", Globals::instance().getActionData(ActionType::RotateUp)->toString().c_str(), 
																							Globals::instance().getActionData(ActionType::RotateDown)->toString().c_str());
			ImGui::Text("Rotate camera left/right             : %s/%s or mouse or r-stick", Globals::instance().getActionData(ActionType::RotateLeft)->toString().c_str(), 
																							Globals::instance().getActionData(ActionType::RotateRight)->toString().c_str());
			ImGui::Text("Move camera forward/backward         : %s/%s or l-stick", Globals::instance().getActionData(ActionType::MoveForward)->toString().c_str(), 
																				   Globals::instance().getActionData(ActionType::MoveBackward)->toString().c_str());
			ImGui::Text("Move camera left / right             : %s/%s or l-stick", Globals::instance().getActionData(ActionType::MoveLeft)->toString().c_str(),
																				   Globals::instance().getActionData(ActionType::MoveRight)->toString().c_str());
			ImGui::Text("Move camera up / down                : %s/%s or l/r-trigger", Globals::instance().getActionData(ActionType::MoveUp)->toString().c_str(),
																					   Globals::instance().getActionData(ActionType::MoveDown)->toString().c_str());
			ImGui::Text("Tilt camera left / right             : %s/%s or d-pad left/right", Globals::instance().getActionData(ActionType::TiltLeft)->toString().c_str(),
																							Globals::instance().getActionData(ActionType::TiltRight)->toString().c_str());
			ImGui::Text("Increase / decrease FoV              : %s/%s or d-pad up/down", Globals::instance().getActionData(ActionType::FovIncrease)->toString().c_str(),
																						 Globals::instance().getActionData(ActionType::FovDecrease)->toString().c_str());
			ImGui::Text("Reset FoV                            : %s or controller B-button", Globals::instance().getActionData(ActionType::FovReset)->toString().c_str());
			ImGui::Text("Block input to game                  : %s", Globals::instance().getActionData(ActionType::BlockInput)->toString().c_str());
			ImGui::Text("Toggle game pause                    : %s", Globals::instance().getActionData(ActionType::Timestop)->toString().c_str());
		}

		if (ImGui::CollapsingHeader("Settings editor help"))
		{
			ImGui::PushTextWrapPos();
			ImGui::TextUnformatted("* Click and drag on lower right corner to resize window.");
			ImGui::TextUnformatted("* Click and drag on any empty space to move window.");
			ImGui::TextUnformatted("* Mouse Wheel to scroll.");
			ImGui::TextUnformatted("* TAB/SHIFT+TAB to cycle through keyboard editable fields.");
			ImGui::TextUnformatted("* Click and drag on a text box to change its value.");
			ImGui::TextUnformatted("* CTRL+Click on a slider or text box to input text.");
			ImGui::TextUnformatted(
				"* While editing text:\n"
				"  - Hold SHIFT or use mouse to select text\n"
				"  - CTRL+Left/Right to word jump\n"
				"  - CTRL+A or double-click to select all\n"
				"  - CTRL+X,CTRL+C,CTRL+V clipboard\n"
				"  - CTRL+Z,CTRL+Y undo/redo\n"
				"  - ESCAPE to revert\n"
				"  - You can apply arithmetic operators +,*,/ on numerical values.\n"
				"    Use +- to subtract.\n");
			ImGui::PopTextWrapPos();
		}
	}


	void renderSettings()
	{
		GameSpecific::CameraManipulator::getSettingsFromGameState();
		Settings& currentSettings = Globals::instance().settings();
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.3f);
		bool settingsChanged = false;
		if (ImGui::Button("Reset to defaults"))
		{
			currentSettings.init(true);
			settingsChanged = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Rehook XInput"))
		{
			InputHooker::setXInputHook(true);
		}
		if (ImGui::CollapsingHeader("Camera movement options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			settingsChanged |= ImGui::SliderFloat("Fast movement multiplier", &currentSettings.fastMovementMultiplier, 0.1f, 100.0f, "%.3f");
			settingsChanged |= ImGui::SliderFloat("Slow movement multiplier", &currentSettings.slowMovementMultiplier, 0.001f, 1.0f, "%.3f");
			settingsChanged |= ImGui::SliderFloat("Up movement multiplier", &currentSettings.movementUpMultiplier, 0.1f, 10.0f, "%.3f");
			settingsChanged |= ImGui::SliderFloat("Movement speed", &currentSettings.movementSpeed, 1.0f, 10.0f, "%.1f");
			settingsChanged |= ImGui::Combo("Camera control device", &currentSettings.cameraControlDevice, "Keyboard & Mouse\0Gamepad\0Both\0\0");
			ImGui::SameLine(); showHelpMarker("The camera control device chosen will be blocked for game input.\n");
			ImGui::TextUnformatted("");  ImGui::SameLine((ImGui::GetWindowWidth() * 0.3f) - 11.0f);
			settingsChanged |= ImGui::Checkbox("Allow camera movement when this menu is up", &currentSettings.allowCameraMovementWhenMenuIsUp);
		}
		if (ImGui::CollapsingHeader("Camera rotation options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			settingsChanged |= ImGui::SliderFloat("Rotation speed", &currentSettings.rotationSpeed, 0.001f, 0.5f, "%.3f");
			ImGui::TextUnformatted("");  ImGui::SameLine((ImGui::GetWindowWidth() * 0.3f)-11.0f);
			settingsChanged |= ImGui::Checkbox("Invert Y look direction", &currentSettings.invertY);
		}
		if (ImGui::CollapsingHeader("Misc. camera options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			settingsChanged |= ImGui::SliderFloat("Field of View (FoV) zoom speed", &currentSettings.fovChangeSpeed, 0.01f, 1.00f, "%.2f");
			ImGui::SliderFloat("Resolution scale factor", &currentSettings.resolutionScale, 25.0f, RESOLUTION_SCALE_MAX, "%.0f");
			ImGui::SameLine(); showHelpMarker("Be careful with values bigger than 200 as it could make\nthe game crash due to too much overhead.\nYou can specify values bigger than 2 by using\nCtrl-click and then type the value.\nMax is 200.");
		}
		ImGui::PopItemWidth();
		if (settingsChanged)
		{
			Globals::instance().markSettingsDirty();
		}
		GameSpecific::CameraManipulator::applySettingsToGameState();
	}


	void renderKeyBindings()
	{
		if (_actionKeyBindingEditing >= 0)
		{
			// a keybinding is being edited. Read current pressed keys into the collector, cumulatively;
			Input::collectPressedKeysCumulatively();
		}
		if (ImGui::CollapsingHeader("Key-binding editing help"))
		{
			ImGui::PushTextWrapPos();
			ImGui::TextUnformatted("Please click inside the textbox of the key-binding you want to change to change it. You can bind multiple actions to the same key combination. A key-binding with no other key than Alt, Shift and/or Ctrl won't be accepted as a valid key-binding.\n\n");
			ImGui::TextUnformatted("NOTE: The 'Alt' key and the 'Right-Ctrl' key are hard-coded bound to augment acceleration of movement/rotation. Using these keys in your key-bindings for movement/rotation actions will therefore not have the desired effect. If you need to use a special key with the movement/rotation actions, use Shift instead. E.g. Shift-Up to move forward, Up for rotate up");
			ImGui::PopTextWrapPos();
		}
		if (ImGui::CollapsingHeader("Configurable key-bindings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);
			for (short i = 0; i < (short)ActionType::Amount; i++)
			{
				ActionData* action = Globals::instance().getActionData(static_cast<ActionType>(i));
				if (nullptr == action)
				{
					continue;
				}
				string textBoxContents = (_actionKeyBindingEditing == i) ? Globals::instance().getKeyCollector().toString() : action->toString();	// The 'press a key' is inside ActionData.
				ImGui::Text("%s", action->getDescription().c_str());
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.4f);
				string keyBindingInputName = "##" + action->getName();	// a label starting with "##" is considered 'hidden' and can be used to add multiple elements in a list. 
				ImGui::InputText(keyBindingInputName.c_str(), (char*)textBoxContents.c_str(), textBoxContents.size(), ImGuiInputTextFlags_ReadOnly);
				if (ImGui::IsItemClicked())
				{
					startKeyBindingCapturing(i);
				}
				if (_actionKeyBindingEditing == i)
				{
					ImGui::SameLine();
					// binding is being edited. This is ok, there's just 1 binding being edited at a time. 
					if (ImGui::Button("OK"))
					{
						endKeyBindingCapturing(true);
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						endKeyBindingCapturing(false);
					}
				}
			}
			ImGui::PopItemWidth();
		}
	}


	void renderNotifications()
	{
		EnterCriticalSection(&_notificationCriticalSection);
			updateNotificationStore();
			if (_activeNotifications.size() > 0)
			{
				float yCoord = 10;
				if (_timeSplashFirstShown >= 0.0f && _displaySplash)
				{
					yCoord = _splashWindowSize.y + 5.0f;	// add a tiny space between splash and the notifications.
				}
				ImGui::SetNextWindowPos(ImVec2(10, yCoord));
				if (ImGui::Begin("Notifications", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
				{
					for (vector<Notification>::iterator it = _activeNotifications.begin(); it < _activeNotifications.end(); it++)
					{
						ImGui::Text(it->notificationText.c_str());
					}
				}
				ImGui::End();
			}
		LeaveCriticalSection(&_notificationCriticalSection);
	}


	// will purge all outdated notifications from the set of active notifications. Be sure to call this from within a mutex lock, as notifications are added from another thread
	void updateNotificationStore()
	{
		float currentRenderTime = ImGui::GetTime();		// time overall,  in seconds

		// for now we'll use 2 seconds (2.0f). First purge everything that's first rendered more than delta T ago
		float maxTimeOnScreen = 2.0f;
		_activeNotifications.erase(remove_if(_activeNotifications.begin(),
											_activeNotifications.end(),
											[currentRenderTime, maxTimeOnScreen](Notification const & x) -> bool 
											{
												return (x.timeFirstDisplayed > 0.0f && (currentRenderTime - x.timeFirstDisplayed) > maxTimeOnScreen); 
											}), 
									_activeNotifications.end());
		// now update all notifications which have a negative time (so haven't been shown) and set them to the current time.
		for_each(begin(_activeNotifications), end(_activeNotifications), 
			[currentRenderTime](Notification & x) 
			{ 
				if (x.timeFirstDisplayed < 0.0f) 
				{
					x.timeFirstDisplayed = currentRenderTime;
				}
			});
	}


	void renderSplash()
	{
		float currentRenderTime = ImGui::GetTime();		// time overall,  in seconds
		if (_timeSplashFirstShown < 0.0f)
		{
			_timeSplashFirstShown = currentRenderTime;
		}
		_displaySplash = _displaySplash & ((currentRenderTime - _timeSplashFirstShown) < IGCS_SPLASH_DURATION);
		if (!_displaySplash)
		{
			return;
		}
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		if (!ImGui::Begin("IGCS Splash", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | 
												  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::End();
			return;
		}
		ImVec4 col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Text("Injectable Camera Tools v%s for %s", CAMERA_VERSION, GAME_NAME);
		ImGui::Text("Camera credits: %s", CAMERA_CREDITS);
		ImGui::Text("\nPress %s to open the main window.\n", Globals::instance().getActionData(ActionType::ToggleOverlay)->toString().c_str());
		ImGui::PopStyleColor();
		ImGui::Separator();
		col = ImVec4(1.0f, 1.0f, 1.0f, 0.6f);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Text("Powered by Injectable Generic Camera System by Otis_Inf");
		ImGui::Text("Get your copy at: https://github.com/FransBouma/InjectableGenericCameraSystem");
		ImGui::PopStyleColor();
		_splashWindowSize = ImGui::GetWindowSize();
		ImGui::End();
	}


	void toggleOverlay()
	{
		_showMainWindow = !_showMainWindow;
	}


	bool isMainMenuVisible()
	{
		return _showMainWindow;
	}

	void showHelpMarker(const char* desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(450.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}


	void startKeyBindingCapturing(short actionType)
	{
		if (_actionKeyBindingEditing == actionType)
		{
			return;
		}
		if (_actionKeyBindingEditing >= 0)
		{
			endKeyBindingCapturing(false);
		}
		_actionKeyBindingEditing = actionType;
	}


	void endKeyBindingCapturing(bool acceptCollectedBinding)
	{
		ActionData& keyCollector = Globals::instance().getKeyCollector();
		if (acceptCollectedBinding && _actionKeyBindingEditing >= 0 && keyCollector.isValid())
		{
			Globals::instance().updateActionDataForAction(static_cast<ActionType>((short)_actionKeyBindingEditing));
		}
		_actionKeyBindingEditing = -1;
		keyCollector.clear();
	}


	void initImGui()
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.IniFilename = IGCS_OVERLAY_INI_FILENAME;
		ImGui::StyleColorsDark();
		initImGuiStyle();
		ImGui_ImplWin32_Init(IGCS::Globals::instance().mainWindowHandle());
	}


	void initImGuiStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.WindowRounding = 2.0f;
		style.FrameRounding = 1.0f;
		style.ScrollbarSize = 16.0f;
		style.ScrollbarRounding = 2.0f;

		style.Colors[ImGuiCol_Text] = ImVec4(0.84f, 0.84f, 0.88f, 1.00f);
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.29f, 1.00f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.90f);
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.3125f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.22f, 0.24f, 0.31f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.00f);
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.27f, 0.27f, 0.33f, 0.37f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.37f, 0.37f, 0.42f, 0.42f);
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.45f, 0.45f, 0.45f, 0.30f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.53f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.65f, 0.31f, 0.00f, 0.71f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.65f, 0.31f, 0.00f, 0.86f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.41f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.50f, 0.50f, 0.53f, 0.49f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.49f, 1.00f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.44f, 0.31f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.44f, 0.44f, 0.44f, 0.30f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
		style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	}
}

