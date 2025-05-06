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

using System.Collections.Generic;
using IGCSClient.Classes;
using IGCSClient.Classes.Settings;

namespace IGCSClient.GameSpecific.Classes
{
	/// <summary>
	/// Creates setting instances for the setting editor. It defines the setting defaults/min/max values for the various settings based
	/// on the game it will be used with.
	/// </summary>
	public static class SettingFactory
	{
		public static void InitializeSettings()
		{
			var appState = AppStateSingleton.Instance();
			appState.AddSetting(new FloatSetting(SettingType.FastMovementMultiplier, nameof(SettingType.FastMovementMultiplier), 2.0, 40.0, 0, 2.0, GameSpecificSettingDefaults.FastMovementMultiplier));
			appState.AddSetting(new FloatSetting(SettingType.SlowMovementMultiplier, nameof(SettingType.SlowMovementMultiplier), 0.005, 1.0, 3, 0.005, GameSpecificSettingDefaults.SlowMovementMultiplier));
			appState.AddSetting(new FloatSetting(SettingType.UpMovementMultiplier, nameof(SettingType.UpMovementMultiplier), 0.5, 10.0, 2, 0.1, GameSpecificSettingDefaults.UpMovementMultiplier));
			appState.AddSetting(new FloatSetting(SettingType.MovementSpeed, nameof(SettingType.MovementSpeed), 0.001, 0.2, 3, 0.001, GameSpecificSettingDefaults.MovementSpeed));
			appState.AddSetting(new DropDownSetting(SettingType.CameraControlDevice, nameof(SettingType.CameraControlDevice),
													new List<string>()
													{
														nameof(CameraDeviceType.KeyboardMouse), nameof(CameraDeviceType.Gamepad), nameof(CameraDeviceType.Both)
													}, GameSpecificSettingDefaults.CameraControlDevice));
			appState.AddSetting(new FloatSetting(SettingType.RotationSpeed, nameof(SettingType.RotationSpeed), 0.001, 0.025, 3, 0.001, GameSpecificSettingDefaults.RotationSpeed));
			appState.AddSetting(new BoolSetting(SettingType.InvertYLookDirection, nameof(SettingType.InvertYLookDirection), GameSpecificSettingDefaults.InvertYLookDirection));
			appState.AddSetting(new FloatSetting(SettingType.FoVZoomSpeed, nameof(SettingType.FoVZoomSpeed), 0.0001, 0.01, 4, 0.0001, GameSpecificSettingDefaults.FoVZoomSpeed));
            appState.AddSetting(new FloatSetting(SettingType.gameSpeed, nameof(SettingType.gameSpeed), 0.05, 2.0, 2, 0.05, GameSpecificSettingDefaults.gameSpeed));
            //Path controller
            appState.AddSetting(new IntSetting(SettingType.PathDuration, nameof(SettingType.PathDuration), 1, 500, 1, GameSpecificSettingDefaults.PathDuration));
            appState.AddSetting(new FloatSetting(SettingType.PathEasingValue, nameof(SettingType.PathEasingValue), 1.0, 3.0, 2, 0.01, GameSpecificSettingDefaults.PathEasingValue));
            appState.AddSetting(new BoolSetting(SettingType.UniformParam, nameof(SettingType.UniformParam), GameSpecificSettingDefaults.UniformParam));
            appState.AddSetting(new BoolSetting(SettingType.DeltaType, nameof(SettingType.DeltaType), GameSpecificSettingDefaults.DeltaType));
			appState.AddSetting(new FloatSetting(SettingType.DeltaValue, nameof(SettingType.DeltaValue), 0.0001, 0.032, 4, 0.0001, GameSpecificSettingDefaults.DeltaValue));
            appState.AddSetting(new DropDownSetting(SettingType.EulerOrder, nameof(SettingType.EulerOrder), new List<string> { "XYZ", "XZY","YXZ","YZX","ZXY","ZYX" }, 2));
            appState.AddSetting(new ToggleSetting(SettingType.PathToggleInterpolationMode, nameof(SettingType.PathToggleInterpolationMode), new List<string> { "CatmullRom", "Centripetal", "Bezier", "BSpline", "Monotonic Cubic"}, null ,0));
            appState.AddSetting(new ToggleSetting(SettingType.RotationMode, nameof(SettingType.RotationMode), new List<string> { "Standard", "SQUAD", "Riemann Cubic"}, null, 0));
            appState.AddSetting(new BoolSetting(SettingType.CameraShakeToggle, nameof(SettingType.CameraShakeToggle), GameSpecificSettingDefaults.CameraShakeToggle));
            appState.AddSetting(new FloatSetting(SettingType.Amplitude, nameof(SettingType.Amplitude), 0.0, 1.0, 3, 0.001, GameSpecificSettingDefaults.Amplitude));
            appState.AddSetting(new FloatSetting(SettingType.Frequency, nameof(SettingType.Frequency), 0.0, 10.0, 2, 0.01, GameSpecificSettingDefaults.Frequency));
            appState.AddSetting(new BoolSetting(SettingType.HandheldCameraToggle, nameof(SettingType.HandheldCameraToggle), GameSpecificSettingDefaults.HandheldCameraToggle));
            appState.AddSetting(new FloatSetting(SettingType.HandheldIntensity, nameof(SettingType.HandheldIntensity), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldIntensity));
            appState.AddSetting(new FloatSetting(SettingType.HandheldDriftIntensity, nameof(SettingType.HandheldDriftIntensity), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldDriftIntensity));
            appState.AddSetting(new FloatSetting(SettingType.HandheldDriftSpeed, nameof(SettingType.HandheldDriftSpeed), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldDriftSpeed));
            appState.AddSetting(new FloatSetting(SettingType.HandheldRotationDriftSpeed, nameof(SettingType.HandheldRotationDriftSpeed), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldRotationDriftSpeed));
            appState.AddSetting(new FloatSetting(SettingType.HandheldJitterIntensity, nameof(SettingType.HandheldJitterIntensity), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldJitterIntensity));
            appState.AddSetting(new FloatSetting(SettingType.HandheldBreathingIntensity, nameof(SettingType.HandheldBreathingIntensity), 0.00, 3.0, 3, 0.001, GameSpecificSettingDefaults.HandheldBreathingIntensity));
            appState.AddSetting(new FloatSetting(SettingType.HandheldBreatingRate, nameof(SettingType.HandheldBreatingRate), 0.00, 20.0, 1, 0.1, GameSpecificSettingDefaults.HandheldBreatingRate));
            appState.AddSetting(new BoolSetting(SettingType.HandheldPositionToggle, nameof(SettingType.HandheldPositionToggle), GameSpecificSettingDefaults.HandheldPositionToggle));
            appState.AddSetting(new BoolSetting(SettingType.HandheldRotationToggle, nameof(SettingType.HandheldRotationToggle), GameSpecificSettingDefaults.HandheldRotationToggle));
            appState.AddSetting(new BoolSetting(SettingType.PlayerRelativeToggle, nameof(SettingType.PlayerRelativeToggle), GameSpecificSettingDefaults.PlayerRelativeToggle));
            appState.AddSetting(new ToggleSetting(SettingType.PathEasingType, nameof(SettingType.PathEasingType), new List<string> { "No Easing", "Ease-in", "Ease-out", "Ease-in-out" }, null, GameSpecificSettingDefaults.PathToggleEasing));
            appState.AddSetting(new ToggleSetting(SettingType.PathSampleCount, nameof(SettingType.PathSampleCount), new List<string> { "64", "128", "256", "512", "1028", "2096"  }, new List<int> { 64, 128, 256, 512, 1028, 2096 }, GameSpecificSettingDefaults.PathSampleCount));
            appState.AddSetting(new FloatSetting(SettingType.PlayerRelativeSmoothness, nameof(SettingType.PlayerRelativeSmoothness), 0.0, 20.0, 1, 0.1, GameSpecificSettingDefaults.PlayerRelativeSmoothness));
            appState.AddSetting(new BoolSetting(SettingType.WaitBeforePlaying, nameof(SettingType.WaitBeforePlaying), GameSpecificSettingDefaults.WaitBeforePlaying));
            appState.AddSetting(new BoolSetting(SettingType.UnpauseOnPlay, nameof(SettingType.UnpauseOnPlay), GameSpecificSettingDefaults.UnpauseOnPlay));
            appState.AddSetting(new BoolSetting(SettingType.VisualisationToggle, nameof(SettingType.VisualisationToggle), GameSpecificSettingDefaults.VisualisationToggle));
            appState.AddSetting(new BoolSetting(SettingType.VisualiseAllPaths, nameof(SettingType.VisualiseAllPaths), GameSpecificSettingDefaults.VisualiseAllPaths));
            appState.AddSetting(new FloatSetting(SettingType.MovementSmoothness, nameof(SettingType.MovementSmoothness), 0.5, 20.0, 1, 0.5, GameSpecificSettingDefaults.MovementSmoothness));
            appState.AddSetting(new FloatSetting(SettingType.RotationSmoothness, nameof(SettingType.RotationSmoothness), 0.5, 20.0, 1, 0.5, GameSpecificSettingDefaults.RotationSmoothness));
            appState.AddSetting(new FloatSetting(SettingType.FOVSmoothness, nameof(SettingType.FOVSmoothness), 0.5, 20.0, 1, 0.5, GameSpecificSettingDefaults.FOVSmoothness));
            appState.AddSetting(new FloatSetting(SettingType.ScrubbingProgress, nameof(SettingType.ScrubbingProgress), 0.0, 1.0, 2, 0.01, GameSpecificSettingDefaults.ScrubbingProgress));
            appState.AddSetting(new BoolSetting(SettingType.D3DDisabled, nameof(SettingType.D3DDisabled), GameSpecificSettingDefaults.D3DDisabled));
            //Game specific
            appState.AddSetting(new BoolSetting(SettingType.BloomToggle, nameof(SettingType.BloomToggle), GameSpecificSettingDefaults.BloomToggle));
            appState.AddSetting(new BoolSetting(SettingType.VignetteToggle, nameof(SettingType.VignetteToggle), GameSpecificSettingDefaults.VignetteToggle));
            appState.AddSetting(new BoolSetting(SettingType.HidePlayer, nameof(SettingType.HidePlayer), GameSpecificSettingDefaults.HidePlayer));
            appState.AddSetting(new BoolSetting(SettingType.HideNPC, nameof(SettingType.HideNPC), GameSpecificSettingDefaults.HideNPC));
            appState.AddSetting(new BoolSetting(SettingType.TimestopType, nameof(SettingType.TimestopType), GameSpecificSettingDefaults.TimestopType));
        }


		public static void InitializeKeyBindings()
		{
			var appState = AppStateSingleton.Instance();
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.BlockInputToGame, nameof(KeyBindingType.BlockInputToGame), new KeyCombination(GameSpecificKeyBindingDefaults.BlockInputToGameDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.EnableDisableCamera, nameof(KeyBindingType.EnableDisableCamera), new KeyCombination(GameSpecificKeyBindingDefaults.EnableDisableCameraDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.DecreaseFoV, nameof(KeyBindingType.DecreaseFoV), new KeyCombination(GameSpecificKeyBindingDefaults.DecreaseFoVDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.IncreaseFoV, nameof(KeyBindingType.IncreaseFoV), new KeyCombination(GameSpecificKeyBindingDefaults.IncreaseFoVDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.ResetFoV, nameof(KeyBindingType.ResetFoV), new KeyCombination(GameSpecificKeyBindingDefaults.ResetFoVDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.LockUnlockCameraMovement, nameof(KeyBindingType.LockUnlockCameraMovement), new KeyCombination(GameSpecificKeyBindingDefaults.LockUnlockCameraMovementDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraLeft, nameof(KeyBindingType.MoveCameraLeft), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraLeftDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraRight, nameof(KeyBindingType.MoveCameraRight), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraRightDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraForward, nameof(KeyBindingType.MoveCameraForward), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraForwardDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraBackward, nameof(KeyBindingType.MoveCameraBackward), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraBackwardDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraUp, nameof(KeyBindingType.MoveCameraUp), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraUpDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.MoveCameraDown, nameof(KeyBindingType.MoveCameraDown), new KeyCombination(GameSpecificKeyBindingDefaults.MoveCameraDownDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.RotateCameraUp, nameof(KeyBindingType.RotateCameraUp), new KeyCombination(GameSpecificKeyBindingDefaults.RotateCameraUpDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.RotateCameraDown, nameof(KeyBindingType.RotateCameraDown), new KeyCombination(GameSpecificKeyBindingDefaults.RotateCameraDownDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.RotateCameraLeft, nameof(KeyBindingType.RotateCameraLeft), new KeyCombination(GameSpecificKeyBindingDefaults.RotateCameraLeftDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.RotateCameraRight, nameof(KeyBindingType.RotateCameraRight), new KeyCombination(GameSpecificKeyBindingDefaults.RotateCameraRightDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.TiltCameraLeft, nameof(KeyBindingType.TiltCameraLeft), new KeyCombination(GameSpecificKeyBindingDefaults.TiltCameraLeftDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.TiltCameraRight, nameof(KeyBindingType.TiltCameraRight), new KeyCombination(GameSpecificKeyBindingDefaults.TiltCameraRightDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PauseUnpauseGame, nameof(KeyBindingType.PauseUnpauseGame), new KeyCombination(GameSpecificKeyBindingDefaults.PauseUnpauseGameDefault)));
			appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.SkipFrames, nameof(KeyBindingType.SkipFrames), new KeyCombination(GameSpecificKeyBindingDefaults.SkipFramesDefault)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.SlowMo, nameof(KeyBindingType.SlowMo), new KeyCombination(GameSpecificKeyBindingDefaults.SlowMoDefault)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.HUDtoggle, nameof(KeyBindingType.HUDtoggle), new KeyCombination(GameSpecificKeyBindingDefaults.HUDtoggle)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.CycleDepthBuffers, nameof(KeyBindingType.CycleDepthBuffers), new KeyCombination(GameSpecificKeyBindingDefaults.CycleDepthBuffers)));
            //Game Specific
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.playerOnlyToggle, nameof(KeyBindingType.playerOnlyToggle), new KeyCombination(GameSpecificKeyBindingDefaults.PlayerOnlyToggle)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathCreate, nameof(KeyBindingType.PathCreate), new KeyCombination(GameSpecificKeyBindingDefaults.PathCreate)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathPlayStop, nameof(KeyBindingType.PathPlayStop), new KeyCombination(GameSpecificKeyBindingDefaults.PathPlayStop)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathAddNode, nameof(KeyBindingType.PathAddNode), new KeyCombination(GameSpecificKeyBindingDefaults.PathAddNode)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathVisualizationToggle, nameof(KeyBindingType.PathVisualizationToggle), new KeyCombination(GameSpecificKeyBindingDefaults.PathVisualizationToggle)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathDelete, nameof(KeyBindingType.PathDelete), new KeyCombination(GameSpecificKeyBindingDefaults.PathDelete)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathCycle, nameof(KeyBindingType.PathCycle), new KeyCombination(GameSpecificKeyBindingDefaults.PathCycle)));
            appState.AddKeyBinding(new KeyBindingSetting(KeyBindingType.PathDeleteLastNode, nameof(KeyBindingType.PathDeleteLastNode), new KeyCombination(GameSpecificKeyBindingDefaults.PathDeleteLastNode)));

            //pathcontroller
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathCreate, nameof(KeyBindingType.PathCreate), GameSpecificKeyBindingDefaults.PathCreate_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathPlayStop, nameof(KeyBindingType.PathPlayStop), GameSpecificKeyBindingDefaults.PathPlayStop_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathAddNode, nameof(KeyBindingType.PathAddNode), GameSpecificKeyBindingDefaults.PathAddNode_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathVisualizationToggle, nameof(KeyBindingType.PathVisualizationToggle), GameSpecificKeyBindingDefaults.PathVisualizationToggle_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathDelete, nameof(KeyBindingType.PathDelete), GameSpecificKeyBindingDefaults.PathDelete_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathCycle, nameof(KeyBindingType.PathCycle), GameSpecificKeyBindingDefaults.PathCycle_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.PathDeleteLastNode, nameof(KeyBindingType.PathDeleteLastNode), GameSpecificKeyBindingDefaults.PathDeleteLastNode_GamepadDefault));


            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.DecreaseFoV, nameof(KeyBindingType.DecreaseFoV), GameSpecificKeyBindingDefaults.DecreaseFoV_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.IncreaseFoV, nameof(KeyBindingType.IncreaseFoV), GameSpecificKeyBindingDefaults.IncreaseFoV_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.ResetFoV, nameof(KeyBindingType.ResetFoV), GameSpecificKeyBindingDefaults.ResetFoV_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.TiltCameraLeft, nameof(KeyBindingType.TiltCameraLeft), GameSpecificKeyBindingDefaults.TiltCameraLeft_GamepadDefault));
            appState.AddGamepadBinding(new GamepadBindingSetting(KeyBindingType.TiltCameraRight, nameof(KeyBindingType.TiltCameraRight), GameSpecificKeyBindingDefaults.TiltCameraRight_GamepadDefault));
        }

	}
}
