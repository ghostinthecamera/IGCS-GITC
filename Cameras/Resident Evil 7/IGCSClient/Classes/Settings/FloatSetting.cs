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

using System.Globalization;
using IGCSClient.Controls;
using IGCSClient.Interfaces;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Represents a float setting including control
	/// </summary>
	public class FloatSetting : Setting<float>
	{
		#region Members
		private readonly double _minValue;
		private readonly double _maxValue;
		private readonly int _scale;
		private readonly double _increment;
		private readonly float _defaultValue;
        private readonly double _tickInterval; // Optional - if not provided, uses increment
        private readonly bool _useFlexibleTicks;
        private bool _isPersistent;
        #endregion


        public FloatSetting(byte id, string name, double minValue, double maxValue, int scale, double increment, float defaultValue, bool isPersistent = true)
			: base(id, name, SettingKind.NormalSetting, isPersistent)
		{
            _minValue = minValue;
            _maxValue = maxValue;
            _scale = scale;
            _increment = increment;
            _tickInterval = increment; // Same as increment for backward compatibility
            _defaultValue = defaultValue;
            _useFlexibleTicks = false;
        }

        public FloatSetting(byte id, string name, double minValue, double maxValue, int scale,
            double stepSize, double tickInterval, float defaultValue, bool isPersistent = true)
            : base(id, name, SettingKind.NormalSetting, isPersistent)
        {
            _minValue = minValue;
            _maxValue = maxValue;
            _scale = scale;
            _increment = stepSize;
            _tickInterval = tickInterval;
            _defaultValue = defaultValue;
            _useFlexibleTicks = true;
        }

        //      public override void Setup(IInputControl<float> controlToUse)
        //{
        //	base.Setup(controlToUse);
        //	var controlAsFloatInput = controlToUse as IFloatSettingControl;
        //	if(controlAsFloatInput == null)
        //	{
        //		return;
        //	}
        //	controlAsFloatInput.Setup(_minValue, _maxValue, _scale, _increment, _defaultValue);
        //	controlAsFloatInput.Value = _defaultValue;
        //}

        public override void Setup(IInputControl<float> controlToUse)
        {
            base.Setup(controlToUse);
            var controlAsFloatInput = controlToUse as IFloatSettingControl;
            if (controlAsFloatInput == null)
            {
                return;
            }

            if (_useFlexibleTicks && controlAsFloatInput is FloatInputSliderWPF enhancedControl)
            {
                // Use enhanced setup with separate tick control
                enhancedControl.SetupWithTickControl(_minValue, _maxValue, _scale, _increment, _tickInterval, _defaultValue);
            }
            else
            {
                // Use original setup method
                controlAsFloatInput.Setup(_minValue, _maxValue, _scale, _increment, _defaultValue);
            }

            controlAsFloatInput.Value = _defaultValue;
        }

        protected override float GetDefaultValue()
		{
			return _defaultValue;
		}


		protected override string GetValueAsString()
		{
			return this.Value.ToString(CultureInfo.InvariantCulture.NumberFormat);
		}
    }
}
