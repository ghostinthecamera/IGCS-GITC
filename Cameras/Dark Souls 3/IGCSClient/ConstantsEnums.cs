﻿////////////////////////////////////////////////////////////////////////////////////////////////////////
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

namespace IGCSClient
{
	internal class ConstantsEnums
	{
		internal static readonly string DllToClientNamedPipeName = "IgcsDllToClient";
		internal static readonly string ClientToDllNamedPipeName = "IgcsClientToDll";
		internal static readonly int BufferLength = 10*1024;	// Up from 4KB, 10KB buffer should be more than enough to account for path data. We expect to be actively reading whenever things arrive.
		internal static readonly string IniFilename = "IGCSClientSettings.ini";
		internal static readonly string IGCSSettingsFolder = "IGCS";
		internal static readonly string RecentlyUsedFilename = "IGCSClientRecentlyUsed.txt";
		internal static readonly int NumberOfDllCacheEntriesToKeep = 100;
		internal static readonly string IGCSRootURL = @"https://github.com/FransBouma/InjectableGenericCameraSystem/";
		internal static readonly int NumberOfResolutionsToKeep = 10;
	}


	/// <summary>
	/// Enum to define the payload type. We support a limited number of value types to send over the pipe. We keep things simple for now.
	/// </summary>
	public enum PayloadType : byte
	{
		Undefined = 0,
		Byte,			// 1 byte length
		Int16,			// 2 byte length
		Int32,			// 4 byte length
		Int64,			// 8 byte length
		AsciiString,	// string length + 1 byte length
		Float,			// 4 byte length, IEEE encoded
		Double,			// 8 byte length, IEEE encoded
		Bool,			// 1 byte length, 1 == true, 0 == false.
	}


	public class MessageType
	{
		public const byte Setting = 1;
		public const byte KeyBinding = 2;
		public const byte Notification = 3;
		public const byte NormalTextMessage = 4;
		public const byte ErrorTextMessage = 5;
		public const byte DebugTextMessage = 6;
		public const byte Action = 7;
        public const byte CameraPathData = 8;
		public const byte UpdatePathPlaying = 9;
		public const byte CameraEnabled = 10;
        public const byte CameraPathBinaryData = 11;
        public const byte UpdateVisualisation = 12;
        public const byte UpdateSelectedPath = 13;
        public const byte CyclePath = 14;
        public const byte PathProgress = 15;
        public const byte NotificationOnly = 16;
    }


	public class CameraDeviceType
	{
		public const byte KeyboardMouse = 0;
		public const byte Gamepad = 1;
		public const byte Both = 2;
	}


	public class SettingType
	{
		public const byte FastMovementMultiplier = 0;
		public const byte SlowMovementMultiplier = 1;
		public const byte UpMovementMultiplier = 2;
		public const byte MovementSpeed = 3;
		public const byte CameraControlDevice = 4;
		public const byte RotationSpeed = 5;
		public const byte InvertYLookDirection = 6;
		public const byte FoVZoomSpeed = 7;
		public const byte gameSpeed = 8;
        public const byte MovementSmoothness = 9;
        public const byte RotationSmoothness = 10;
		public const byte FOVSmoothness = 11;
		//gamespecific
        public const byte BloomToggle = 12;
        public const byte VignetteToggle = 13;
        public const byte HidePlayer = 14;
        public const byte HideNPC = 15;
        //temp path controller vars
        public const byte PathDuration = 16;
        public const byte PathEasingValue = 17;
        public const byte PathSampleCount = 18;
        public const byte PathToggleEasing = 19;
        public const byte PathToggleInterpolationMode = 20;
        public const byte PathEasingType = 21;
        public const byte PathAlphaValue = 22;
		public const byte EulerOrder = 23;
        public const byte UniformParam = 24;
        public const byte RotationMode = 25;
		public const byte DeltaType = 26;
		public const byte DeltaValue = 27;
		public const byte CameraShakeToggle = 28;
		public const byte Amplitude = 29;
        public const byte Frequency = 30;
        public const byte HandheldCameraToggle = 31;
        public const byte HandheldIntensity = 32;
        public const byte HandheldDriftIntensity = 33;
		public const byte HandheldJitterIntensity = 34;
		public const byte HandheldBreathingIntensity = 35;
		public const byte HandheldBreatingRate = 36;
		public const byte HandheldRotationToggle = 37;
		public const byte HandheldPositionToggle = 38;
        public const byte PlayerRelativeToggle = 39;
        public const byte PlayerRelativeSmoothness = 40;
		public const byte WaitBeforePlaying = 41;
        public const byte UnpauseOnPlay = 42;
		public const byte HandheldDriftSpeed = 43;
        public const byte HandheldRotationDriftSpeed = 44;
		public const byte VisualisationToggle = 45;
		public const byte VisualiseAllPaths = 46;
        public const byte ScrubbingProgress = 47;
        public const byte D3DDisabled = 48;
        public const byte TimestopType = 49;
        // to add more, derived a type of this class and define the next value one higher than the last one in this class.
    }


	public class KeyBindingType
    {
		public const byte BlockInputToGame = 0;
		public const byte EnableDisableCamera = 1;
		public const byte DecreaseFoV=2;
		public const byte IncreaseFoV=3;
		public const byte ResetFoV=4;
		public const byte LockUnlockCameraMovement=5;
		public const byte MoveCameraLeft=6;
		public const byte MoveCameraRight=7;
		public const byte MoveCameraForward=8;
		public const byte MoveCameraBackward=9;
		public const byte MoveCameraUp=10;
		public const byte MoveCameraDown=11;
		public const byte RotateCameraUp=12;
		public const byte RotateCameraDown=13;
		public const byte RotateCameraLeft=14;
		public const byte RotateCameraRight=15;
		public const byte TiltCameraLeft=16;
		public const byte TiltCameraRight=17;
		public const byte PauseUnpauseGame = 18;
		public const byte SkipFrames = 19;
		public const byte SlowMo = 20;
        public const byte HUDtoggle = 21;
		public const byte playerOnlyToggle = 22;
        public const byte CycleDepthBuffers = 23;

        // Path controls
        public const byte PathVisualizationToggle = 24;
        public const byte PathPlayStop = 25;
        public const byte PathAddNode = 26;
        public const byte PathCreate = 27;
        public const byte PathDelete = 28;
        public const byte PathCycle = 29;
        public const byte PathDeleteLastNode = 30;

        public const byte ToggleDepthBuffer = 31;
        // to add more, derived a type of this class and define the next value one higher than the last one in this class.
    }


	public class SettingKind
	{
		public const byte NormalSetting = 1;
		public const byte KeyBinding = 2;
	}


	public class ActionType
	{
		public const byte RehookXInput = 1;
		/// <summary>
		/// action types for hotsampling
		/// </summary>
        public const byte setWidth = 2;
        public const byte setheight = 3;
        public const byte setResolution = 4;
    }

    public class PathActionType
    {
        public const byte addPath = 1;
		public const byte deletePath = 2;
		public const byte addNode = 3;
		public const byte playPath = 4;
		public const byte stopPath = 5;
		public const byte gotoNode = 6;
		public const byte updateNode = 7;
		public const byte pausePathPlayBack = 8;
		public const byte deleteNode = 9;
		public const byte refreshPath = 10;
		public const byte selectedPathUpdate = 11;
        public const byte insertNode = 12;
        public const byte selectedNodeIndexUpdate = 13;
        public const byte pathManagerStatus = 14;
        public const byte pathScrubbing = 15;
        public const byte pathScrubPosition = 16;
    }

	public class PathManagerState
	{
		public const byte Idle = 0;
		public const byte PlayingPath = 1;
		public const byte gotoNode = 2;
        public const byte Scrubbing = 3;
    }
}
