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
using System.Windows;
using System.Windows.Controls;
using IGCSClient.Classes;
using IGCSClient.Interfaces;
using ModernWpf.Controls;

namespace IGCSClient.Controls
{
	/// <summary>
	/// Int / number editor
	/// </summary>
	public partial class IntInputWPF : UserControl, IInputControl<int>
	{
		private bool _suppressEvents = false;
		private int _defaultValue;

		public event EventHandler ValueChanged;

		public IntInputWPF()
		{
			InitializeComponent();
			DataObject.AddCopyingHandler(_numberControl, (s, e) => { if (e.IsDragDrop) e.CancelCommand(); });
		}


		public void Setup(int minValue, int maxValue, int increment, int defaultValue)
		{
			// Necessary to suppress events as setting the minimum sets the value too.
			_suppressEvents = true;
			_numberControl.Maximum = maxValue;
			_numberControl.Minimum = minValue;
			_numberControl.AcceptsExpression = false;
			_numberControl.SmallChange = increment;
			_numberControl.LargeChange = increment;
			_defaultValue = defaultValue;
			_suppressEvents = false;
		}
		
		
		public void SetValueFromString(string valueAsString, int defaultValue)
		{
			if(!Int32.TryParse(valueAsString, out var valueToSet))
			{
				valueToSet = defaultValue;
			}
			this.Value = valueToSet;
		}


		private void _numberControl_OnValueChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
		{
			if(_suppressEvents)
			{
				return;
			}

			try
			{
				var currentValue = this.Value;
			}
			catch
			{
				// overflow, reset to default.
				this.Value = _defaultValue;
				return;
			}
			this.ValueChanged.RaiseEvent(this);
		}


		#region Properties
		/// <inheritdoc/>
		public int Value
		{
			get { return Convert.ToInt32(_numberControl.Value); }
			set { _numberControl.Value = value; }
		}

		public string Header
		{
			get { return _headerControl.Header?.ToString() ?? string.Empty; }
			set { _headerControl.Header = value; }
		}
		#endregion
	}
}
