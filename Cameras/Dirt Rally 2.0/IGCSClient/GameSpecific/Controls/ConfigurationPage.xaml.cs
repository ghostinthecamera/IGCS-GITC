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
using System;
using System.Collections.Generic;
using System.Drawing.Drawing2D;
using System.Windows;
using System.Windows.Controls;
using IGCSClient.Classes;
using IGCSClient.Controls;
using IGCSClient.Forms;
using IGCSClient.Interfaces;

namespace IGCSClient.GameSpecific.Controls
{
	/// <summary>
	/// Editor for various camera settings
	/// </summary>
	public partial class ConfigurationPage : UserControl
	{


        private static readonly Dictionary<string, string> _tooltipTexts = new()
        {
            // Shake & handheld
            { nameof(_enableShakeB),
                "Toggle simple camera shake effect on/off" },
            { nameof(_shakeAmplitudeB),
                "Amplitude of the shake. Larger values create larger movements" },
            { nameof(_shakeFrequencyB),
                "Frequency of the shake. Larger values create more vigorous movements, smaller values create longer movements" },
            { nameof(_enablehandheldB),
                "Toggle handheld‑camera effect on/off" },
            { nameof(_handheldIntensityB),
                "Intensity of the handheld motion. Increasing this generally increases the impact of the effect" },
            { nameof(_handheldDriftIntensityB),
                "Controls the overall impact of drift parameters. Larger values create more pronounced drift" },
            { nameof(_handheldJitterIntensityB),
                "Intensity of handheld jitter (the smaller, sharper motions). Increase for more jitter" },
            { nameof(_handheldBreathingIntensityB),
                "Intensity of breathing motion" },
            { nameof(_handheldBreathingRateB),
                "Rate of breathing motion" },
            { nameof(_handheldPositionToggleB),
                "Toggle handheld effect to position. When enabled it will isolate shake just to camera position" },
            { nameof(_handheldRotationToggleB),
                "Toggle handheld effect to rotation. When enabled it will isolate shake just to camera rotation" },
            { nameof(_handheldDriftSpeedB),
                "Rate of drift for positional component. Increase for faster movements in camera position" },
            { nameof(_handheldRotationDriftSpeedB),
                "Rate of drift for rotational component. Increase for faster movements in camera rotation" },

        };

        private bool GetCameraEnabledStatus()
        {
            return MainWindow.CurrentPathControllerWindow?.PathControllerInstance?.IsCameraEnabled ?? false;
        }

        public ConfigurationPage()
		{
			InitializeComponent();
            foreach (var kv in _tooltipTexts)
            {
                if (FindName(kv.Key) is FrameworkElement fe)
                    ToolTipService.SetToolTip(fe, kv.Value);
            }
        }

        public void toggleMotionBlur(byte enable)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => toggleMotionBlur(enable));
                return;
            }

            _motionBlur.IsEnabled = enable != 0;
        }

        public void UpdateMotionBlurStrength(float strength)
        {
            if (!GetCameraEnabledStatus())
            {
                //return;
            }

            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => UpdateMotionBlurStrength(strength));
                return;
            }

            try
            {
                _motionBlur.SetValueSilently(strength);
            }
            catch (Exception ex)
            {
                // If the value is not settable, it will throw an exception.
                // This can happen if the game does not support motion blur.
                // We can ignore this exception.
                Console.WriteLine($"Error setting motion blur strength: {ex.Message}", "Configuration Page",true,true);
            }

        }


        internal void Setup()
		{
			var settings = AppStateSingleton.Instance().Settings;
			foreach(var setting in settings)
			{
				switch(setting.ID)
				{
					case SettingType.FastMovementMultiplier:
						setting.Setup(_fastMovementInput);
						break;
					case SettingType.SlowMovementMultiplier:
						setting.Setup(_slowMovementInput);
						break;
					case SettingType.UpMovementMultiplier:
						setting.Setup(_upMovementInput);
						break;
					case SettingType.MovementSpeed:
						setting.Setup(_movementSpeedInput);
						break;
					case SettingType.CameraControlDevice:
						setting.Setup(_cameraControlDeviceInput);
						break;
					case SettingType.RotationSpeed:
						setting.Setup(_rotationSpeedInput);
						break;
					case SettingType.InvertYLookDirection:
						setting.Setup(_invertYInput);
						break;
					case SettingType.FoVZoomSpeed:
						setting.Setup(_fovSpeedInput);
						break;
                    case SettingType.gameSpeed:
                        setting.Setup(_gameSpeed);
                        break;
                    case SettingType.MovementSmoothness:
                        setting.Setup(_movementSmoothness);
                        break;
                    case SettingType.RotationSmoothness:
                        setting.Setup(_rotationSmoothness);
                        break;
                    case SettingType.FOVSmoothness:
                        setting.Setup(_fovSmoothness);
                        break;
                    case SettingType.LookAtPlayer:
                        setting.Setup(_toggleLookAt);
                        break;
                    case SettingType.CameraShakeToggleB:
                        setting.Setup(_enableShakeB);
                        break;
                    case SettingType.AmplitudeB:
                        setting.Setup(_shakeAmplitudeB);
                        break;
                    case SettingType.FrequencyB:
                        setting.Setup(_shakeFrequencyB);
                        break;
                    case SettingType.HandheldCameraToggleB:
                        setting.Setup(_enablehandheldB);
                        break;
                    case SettingType.HandheldIntensityB:
                        setting.Setup(_handheldIntensityB);
                        break;
                    case SettingType.HandheldDriftIntensityB:
                        setting.Setup(_handheldDriftIntensityB);
                        break;
                    case SettingType.HandheldJitterIntensityB:
                        setting.Setup(_handheldJitterIntensityB);
                        break;
                    case SettingType.HandheldBreathingIntensityB:
                        setting.Setup(_handheldBreathingIntensityB);
                        break;
                    case SettingType.HandheldBreatingRateB:
                        setting.Setup(_handheldBreathingRateB);
                        break;
                    case SettingType.HandheldPositionToggleB:
                        setting.Setup(_handheldPositionToggleB);
                        break;
                    case SettingType.HandheldRotationToggleB:
                        setting.Setup(_handheldRotationToggleB);
                        break;
                    case SettingType.HandheldDriftSpeedB:
                        setting.Setup(_handheldDriftSpeedB);
                        break;
                    case SettingType.HandheldRotationDriftSpeedB:
                        setting.Setup(_handheldRotationDriftSpeedB);
                        break;
                    case SettingType.DoFToggle:
                        setting.Setup(_dofToggle);
                        break;
                    case SettingType.MotionBlurStrength:
                        setting.Setup(_motionBlur);
                        break;
                }
			}
		}
    }
}