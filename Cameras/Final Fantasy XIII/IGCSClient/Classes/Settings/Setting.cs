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
using System.Windows.Controls;
using IGCSClient.Interfaces;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Defines a setting which is edited in the client and used in the system on the other side of the named pipe.
	/// </summary>
	public class Setting<T> : ISetting
	{
		#region Members
		private IInputControl<T> _inputControl;
		private byte _settingKind;
		#endregion


		/// <summary>
		/// Ctor
		/// </summary>
		/// <param name="id">the id of the setting</param>
		/// <param name="name">the name of the setting</param>
		/// <param name="settingKind">The kind of the setting, which is used for messaging</param>
		public Setting(byte id, string name, byte settingKind)
		{
			this.ID = id;
			this.Name = name;
			_settingKind = settingKind;
		}


		/// <summary>
		/// Sets up the setting with the control specified
		/// </summary>
		/// <param name="controlToUse"></param>
		public virtual void Setup(IInputControl<T> controlToUse)
		{
			ArgumentVerifier.CantBeNull(controlToUse, nameof(controlToUse));
			_inputControl = controlToUse;
			_inputControl.ValueChanged += _inputControl_ValueChanged;
		}


		/// <inheritdoc/>
		public virtual void SendValueAsMessage()
		{
			MessageHandlerSingleton.Instance().SendSettingMessage(this.ID, GeneralUtils.ConvertToByteArray(this.Value));
			LogHandlerSingleton.Instance().LogLine("Setting value sent: ID: {0}. Value: {1}", "Setting", true, this.ID, this.GetValueAsString());
		}


		/// <inheritdoc />
		public void SetValueFromString(string valueAsString)
		{
			_inputControl.SetValueFromString(valueAsString, GetDefaultValue());
		}


		protected virtual T GetDefaultValue()
		{
			return default(T);
		}
		

		protected virtual string GetValueAsString()
		{
			if(!(this.Value is object))
			{
				// null
				return string.Empty;
			}

			return Convert.ToString(this.Value);
		}
		

		private void _inputControl_ValueChanged(object sender, EventArgs e)
		{
			SendValueAsMessage();
		}


		/// <inheritdoc />
		void ISetting.Setup(Control inputControl)
		{
			var inputControlTyped = inputControl as IInputControl<T>;
			if(inputControlTyped == null)
			{
				return;
			}
			this.Setup(inputControlTyped);
		}


		/// <inheritdoc />
		string ISetting.GetValueAsString()
		{
			return this.GetValueAsString();
		}


		#region Properties
		/// <inheritdoc />
		public string Name { get; private set; }
		/// <inheritdoc />
		public byte ID { get; private set; }
		/// <inheritdoc />
		public Control InputControl => _inputControl as Control;
		/// <summary>
		/// The current value of the setting as known by this client.
		/// </summary>
		public T Value => _inputControl.Value;
		#endregion
	}
}
