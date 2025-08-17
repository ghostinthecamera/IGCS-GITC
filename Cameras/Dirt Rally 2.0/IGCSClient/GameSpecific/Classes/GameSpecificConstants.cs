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

using System.ComponentModel;
using System.Windows.Forms;
using IGCSClient.Classes;

namespace IGCSClient.GameSpecific.Classes
{
	internal static class GameSpecificConstants
	{
		public const string ClientWindowTitle = "Dirt Rally 2.0 Tools";
		public const string CameraVersion = "2.0.1";
		public const string CameraCredits = "ghostinthecamera";
		public const bool HotsamplingRequiresEXITSIZEMOVE = false;
	}


	internal static class GameSpecificSettingDefaults
	{
		public const float FastMovementMultiplier = 5.0f;
		public const float SlowMovementMultiplier = 0.100f;
		public const float UpMovementMultiplier = 1.0f;
		public const float MovementSpeed = 0.008f;
		public const int CameraControlDevice = 2;
		public const float RotationSpeed = 0.002f;
		public const bool InvertYLookDirection = false;
		public const float FoVZoomSpeed = 0.0020f;
		public const float gameSpeed = 0.1f;
        public const float MovementSmoothness = 3.0f;
        public const float RotationSmoothness = 3.0f;
		public const float FOVSmoothness = 3.0f;
        //path settings
        public const int PathDuration = 10;
        public const float PathEasingValue = 1.5f;
        public const int PathSampleCount = 128;
        public const int PathToggleEasing = 0;
		public const bool UniformParam = true;
		public const bool DeltaType = false;
		public const float DeltaValue = 0.016f;
		public const bool CameraShakeToggle = false;
        public const float Amplitude = 0.2f;
        public const float Frequency = 0.2f;
        public const float HandheldIntensity = 0.5f;
        public const float HandheldDriftIntensity = 0.05f;
        public const float HandheldJitterIntensity = 0.0f;
        public const float HandheldBreathingIntensity = 0.0f;
        public const float HandheldBreatingRate = 0.0f;
		public const float HandheldDriftSpeed = 0.08f;
		public const float HandheldRotationDriftSpeed = 0.1f;
        public const bool HandheldPositionToggle = false;
        public const bool HandheldRotationToggle = false;
        public const bool HandheldCameraToggle = false;
		public const bool PlayerRelativeToggle = false;
		public const float PlayerRelativeSmoothness = 8.0f;
        public const bool WaitBeforePlaying = false;
        public const bool UnpauseOnPlay = false;
		public const bool VisualisationToggle = false;
		public const bool VisualiseAllPaths = false;
        public const float ScrubbingProgress = 0.0f;
        public const bool D3DDisabled = false; // Default: D3D hooking enabled
        public const bool LookAtPlayer = false;

        public const bool PathLookAtEnabled = false;
        public const float PathLookAtOffsetX = 0.0f; // Offset in the X direction for the look-at target
        public const float PathLookAtOffsetY = 0.0f; // Offset in the Y direction for the look-at target
        public const float PathLookAtOffsetZ = 0.0f; // Offset in the Z direction for the look-at target

        public const float PathLookAtSmoothness = 5.0f;

        public const bool PathSpeedMatchingEnabled = false;
        public const float PathSpeedScale = 1.0f; 
        public const float PathSpeedSmoothness = 5.0f;
        public const float PathMinSpeed = 0.0f; 
        public const float PathMaxSpeed = 3.0f;
        public const float PathBaselineSpeed = 5.0f;

        // Game specific
        public const bool DOFToggle = false;
        public const float MotionBlurStrength = 0.0f;
    }


    internal static class GameSpecificKeyBindingDefaults
    {
        public const int BlockInputToGameDefault = (int)Keys.Decimal;
        public const int EnableDisableCameraDefault = (int)Keys.Insert;
        public const int DecreaseFoVDefault = (int)Keys.Subtract;
        public const int IncreaseFoVDefault = (int)Keys.Add;
        public const int ResetFoVDefault = (int)Keys.Multiply;
        public const int LockUnlockCameraMovementDefault = (int)Keys.Home;
        public const int MoveCameraLeftDefault = (int)Keys.NumPad4;
        public const int MoveCameraRightDefault = (int)Keys.NumPad6;
        public const int MoveCameraForwardDefault = (int)Keys.NumPad8;
        public const int MoveCameraBackwardDefault = (int)Keys.NumPad5;
        public const int MoveCameraUpDefault = (int)Keys.NumPad9;
        public const int MoveCameraDownDefault = (int)Keys.NumPad7;
        public const int RotateCameraUpDefault = (int)Keys.Up;
        public const int RotateCameraDownDefault = (int)Keys.Down;
        public const int RotateCameraLeftDefault = (int)Keys.Left;
        public const int RotateCameraRightDefault = (int)Keys.Right;
        public const int TiltCameraLeftDefault = (int)Keys.NumPad1;
        public const int TiltCameraRightDefault = (int)Keys.NumPad3;
        public const int PauseUnpauseGameDefault = (int)Keys.NumPad0;
        public const int SkipFramesDefault = (int)Keys.PageDown;
        public const int SlowMoDefault = (int)Keys.PageUp;
        public const int HUDtoggle = (int)Keys.Delete;
        public const int ToggleDepthBuffer = (int)Keys.D9;
        public const int CycleDepthBuffers = (int)Keys.Tab;

        // Path controls
        public const int PathCreate = (int)Keys.F2;
        public const int PathPlayStop = (int)Keys.F3;
        public const int PathAddNode = (int)Keys.F4;
        public const int PathVisualizationToggle = (int)Keys.F12;
        public const int PathDelete = (int)Keys.F10;
        public const int PathCycle = (int)Keys.OemCloseBrackets;
        public const int PathDeleteLastNode = (int)Keys.F11;

        public const int ResetLookAtOffsets = (int)Keys.F5;
        public const int ToggleOffsetMode = (int)Keys.F6;
        public const int ToggleFixedCameraMount = (int)Keys.F7;
        public const int ToggleHeightLockedMovement = (int)Keys.F8;
        public const int ToggleLookAtVisualisation = (int)Keys.F9;
        public const int MoveUpTarget = (int)Keys.O;
        public const int MoveDownTarget = (int)Keys.L;

        // Path controls for gamepad
        public static readonly KeyCombination IncreaseFoV_GamepadDefault = new(false, false, false, XInputConstants.DPAD_DOWN, true);
        public static readonly KeyCombination DecreaseFoV_GamepadDefault = new(false, false, false, XInputConstants.DPAD_UP, true);
        public static readonly KeyCombination ResetFoV_GamepadDefault = new(false, false, false, XInputConstants.B, true);
        public static readonly KeyCombination TiltCameraLeft_GamepadDefault = new(false, false, false, XInputConstants.DPAD_LEFT, true);
        public static readonly KeyCombination TiltCameraRight_GamepadDefault = new(false, false, false, XInputConstants.DPAD_RIGHT, true);

        public static readonly KeyCombination PathVisualizationToggle_GamepadDefault = new(false, true, false, XInputConstants.L3, true); // Left Stick Press
        public static readonly KeyCombination PathPlayStop_GamepadDefault = new(false, true, false, XInputConstants.X, true); // X button
        public static readonly KeyCombination PathAddNode_GamepadDefault = new(false, true, false, XInputConstants.A, true);
        public static readonly KeyCombination PathCreate_GamepadDefault = new(false, true, false, XInputConstants.Y, true);
		public static readonly KeyCombination PathDelete_GamepadDefault = new(false, true, false, XInputConstants.B, true);
        public static readonly KeyCombination PathCycle_GamepadDefault = new(false, true, false, XInputConstants.DPAD_RIGHT, true);
        public static readonly KeyCombination PathDeleteLastNode_GamepadDefault = new(false, true, false, XInputConstants.DPAD_LEFT, true);

        // Gamepad Look At Control Defaults
        public static readonly KeyCombination ResetLookAtOffsets_GamepadDefault = new(true, true, false, XInputConstants.B, true);
        public static readonly KeyCombination ToggleOffsetMode_GamepadDefault = new(true, true, false, XInputConstants.Y, true);
        public static readonly KeyCombination ToggleFixedCameraMounte_GamepadDefault = new(true, true, false, XInputConstants.A, true);
        public static readonly KeyCombination ToggleHeightLockedMovement_GamepadDefault = new(true, true, false, XInputConstants.X, true);
        public static readonly KeyCombination ToggleLookAtVisualisation_GamepadDefault = new(true, true, false, XInputConstants.R3, true);

    }
}