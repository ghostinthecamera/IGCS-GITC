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

using IGCSClient.Controls;
using IGCSClient.Interfaces;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Represents a folder/directory setting including control
	/// </summary>
	public class FolderSetting : Setting<string>
	{
		#region Members
		private readonly string _initialFolder;
		private readonly string _description;
		#endregion

		
		public FolderSetting(byte id, string name, string initialFolder, string description)
			: base(id, name, SettingKind.NormalSetting)
		{
			_initialFolder = initialFolder;
			_description = description;
		}


		public override void Setup(IInputControl<string> controlToUse)
		{
			base.Setup(controlToUse);
			var controlAsFolderInput = controlToUse as FolderInputWPF;
			if(controlAsFolderInput == null)
			{
				return;
			}
			controlAsFolderInput.Setup(_initialFolder, _description);
		}


		protected override string GetDefaultValue()
		{
			return _initialFolder;
		}
	}
}
