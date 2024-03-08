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
using IGCSClient.Controls;
using IGCSClient.Interfaces;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Represents a multi-select setting, including control
	/// </summary>
	public class DropDownSetting : Setting<int>
	{
		#region Members
		private List<string> _items;
		private int _defaultValue;
		#endregion

		public DropDownSetting(byte id, string name, List<string> items, int defaultValue)
			: base(id, name, SettingKind.NormalSetting)
		{
			ArgumentVerifier.CantBeNull(items, nameof(items));
			ArgumentVerifier.ShouldBeTrue(l=>l.Count > 0, items, $"{nameof(items)} has to have at least one element");
			ArgumentVerifier.ShouldBeTrue(v=>v>=0, defaultValue, $"{nameof(defaultValue)} should be 0 or higher");
			_items = items;
			_defaultValue = defaultValue;
		}


		public override void Setup(IInputControl<int> controlToUse)
		{
			base.Setup(controlToUse);
			var controlAsDropDown = controlToUse as DropDownInputWPF;
			if(controlAsDropDown == null)
			{
				return;
			}
			controlAsDropDown.Setup(_items);
			controlAsDropDown.Value = _defaultValue;
		}


		protected override int GetDefaultValue()
		{
			return _defaultValue;
		}
	}
}
