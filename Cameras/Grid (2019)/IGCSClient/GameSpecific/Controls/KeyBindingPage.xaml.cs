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
using System.Windows.Controls;
using System.Windows.Input;
using IGCSClient.Classes;

namespace IGCSClient.GameSpecific.Controls
{
	/// <summary>
	/// Editor for keybindings
	/// </summary>
	public partial class KeyBindingPage : UserControl
	{
		public KeyBindingPage()
		{
			InitializeComponent();
		}


		internal void Setup()
		{
			var keybindings = AppStateSingleton.Instance().KeyBindings;
            foreach (var binding in keybindings)
			{
				switch(binding.ID)
				{
					case KeyBindingType.BlockInputToGame:
						binding.Setup(_blockInputInput);
						break;
					case KeyBindingType.EnableDisableCamera:
						binding.Setup(_enableDisableCameraInput);
						break;
					case KeyBindingType.DecreaseFoV:
						binding.Setup(_decreaseFoVInput);
						break;
					case KeyBindingType.IncreaseFoV:
						binding.Setup(_increaseFoVInput);
						break;
					case KeyBindingType.ResetFoV:
						binding.Setup(_resetFoVInput);
						break;
					case KeyBindingType.LockUnlockCameraMovement:
						binding.Setup(_lockUnlockCameraMovementInput);
						break;
					case KeyBindingType.MoveCameraLeft:
						binding.Setup(_moveCameraLeftInput);
						break;
					case KeyBindingType.MoveCameraRight:
						binding.Setup(_moveCameraRightInput);
						break;
					case KeyBindingType.MoveCameraForward:
						binding.Setup(_moveCameraForwardInput);
						break;
					case KeyBindingType.MoveCameraBackward:
						binding.Setup(_moveCameraBackwardInput);
						break;
					case KeyBindingType.MoveCameraUp:
						binding.Setup(_moveCameraUpInput);
						break;
					case KeyBindingType.MoveCameraDown:
						binding.Setup(_moveCameraDownInput);
						break;
					case KeyBindingType.RotateCameraUp:
						binding.Setup(_rotateCameraUpInput);
						break;
					case KeyBindingType.RotateCameraDown:
						binding.Setup(_rotateCameraDownInput);
						break;
					case KeyBindingType.RotateCameraLeft:
						binding.Setup(_rotateCameraLeftInput);
						break;
					case KeyBindingType.RotateCameraRight:
						binding.Setup(_rotateCameraRightInput);
						break;
					case KeyBindingType.TiltCameraLeft:
						binding.Setup(_tiltCameraLeftInput);
						break;
					case KeyBindingType.TiltCameraRight:
						binding.Setup(_tiltCameraRightInput);
						break;
					case KeyBindingType.PauseUnpauseGame:
						binding.Setup(_pauseUnPauseInput);
						break;
					case KeyBindingType.SkipFrames:
						binding.Setup(_skipFramesInput);
						break;
                    case KeyBindingType.SlowMo:
                        binding.Setup(_slowMoToggle);
						break;
                    case KeyBindingType.HUDtoggle:
                        binding.Setup(_HUDtoggle);
                        break;
                    case KeyBindingType.CycleDepthBuffers:
                        binding.Setup(_cycleDepthBuffers);
                        break;
                    case KeyBindingType.PathCreate:
                        binding.Setup(_pathCreateInput);
                        break;
                    case KeyBindingType.PathPlayStop:
                        binding.Setup(_pathPlayStopInput);
                        break;
                    case KeyBindingType.PathAddNode:
                        binding.Setup(_pathAddNodeInput);
                        break;
                    case KeyBindingType.PathVisualizationToggle:
                        binding.Setup(_pathVisualizeToggleInput);
                        break;
                    case KeyBindingType.PathDelete:
                        binding.Setup(_pathDeletePath);
                        break;
                    case KeyBindingType.PathCycle:
                        binding.Setup(_pathCycle);
                        break;
                    case KeyBindingType.PathDeleteLastNode:
                        binding.Setup(_pathDeleteLastNode);
                        break;
					case KeyBindingType.ToggleDepthBuffer:
						binding.Setup(_toggleDepthUsage);
                        break;
                    case KeyBindingType.ResetLookAtOffsets:
                        binding.Setup(_resetOffsetLookAt);
                        break;
                    case KeyBindingType.ToggleOffsetMode:
                        binding.Setup(_toggleOffsetMode);
                        break;
                    case KeyBindingType.ToggleHeightLockedMovement:
                        binding.Setup(_toggleHeightLockedMovement);
                        break;
                    case KeyBindingType.ToggleFixedCameraMount:
                        binding.Setup(_toggleFixedCameraMode);
                        break;
                    case KeyBindingType.ToggleLookAtVisualisation:
                        binding.Setup(_toggleLookAtVisualisation);
                        break;
                    case KeyBindingType.MoveUpTarget:
                        binding.Setup(_moveTargetUp);
                        break;
                    case KeyBindingType.MoveDownTarget:
                        binding.Setup(_moveTargetDown);
                        break;
                }
			}

            // Setup gamepad bindings
            var gamepadBindings = AppStateSingleton.Instance().GamepadBindings;
            foreach (var binding in gamepadBindings)
            {
                switch (binding.ID)
                {
					case KeyBindingType.PathCreate:
						binding.Setup(_pathCreateInputGP);
						break;
                    case KeyBindingType.PathPlayStop:
                        binding.Setup(_pathPlayStopInputGP);
                        break;
                    case KeyBindingType.PathAddNode:
                        binding.Setup(_pathAddNodeInputGP);
                        break;
                    case KeyBindingType.PathVisualizationToggle:
                        binding.Setup(_pathVisualizeToggleInputGP);
                        break;
                    case KeyBindingType.PathDelete:
                        binding.Setup(_pathDeletePathGP);
                        break;
                    case KeyBindingType.PathCycle:
                        binding.Setup(_pathCycleGP);
                        break;
                    case KeyBindingType.PathDeleteLastNode:
                        binding.Setup(_pathDeleteLastNodeGP);
                        break;
                    case KeyBindingType.DecreaseFoV:
                        binding.Setup(_decreaseFoVInputGP);
                        break;
                    case KeyBindingType.IncreaseFoV:
                        binding.Setup(_increaseFoVInputGP);
                        break;
                    case KeyBindingType.ResetFoV:
                        binding.Setup(_resetFoVInputGP);
                        break;
                    case KeyBindingType.TiltCameraLeft:
                        binding.Setup(_tiltCameraLeftInputGP);
                        break;
                    case KeyBindingType.TiltCameraRight:
                        binding.Setup(_tiltCameraRightInputGP);
                        break;
					case KeyBindingType.ResetLookAtOffsets:
						binding.Setup(_resetOffsetLookAtGP);
                        break;
					case KeyBindingType.ToggleOffsetMode:
						binding.Setup(_toggleOffsetModeGP);
                        break;
					case KeyBindingType.ToggleHeightLockedMovement:
                        binding.Setup(_toggleHeightLockedMovementGP);
                        break;
					case KeyBindingType.ToggleFixedCameraMount:
                        binding.Setup(_toggleFixedCameraModeGP);
                        break;
					case KeyBindingType.ToggleLookAtVisualisation:
                        binding.Setup(_toggleLookAtVisualisationGP);
                        break;
                }
            }
        }
    }
}

