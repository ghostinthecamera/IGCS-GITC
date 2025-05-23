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
using System;
using System.Windows;
using System.Windows.Controls;
using IGCSClient.Classes;
using IGCSClient.Interfaces;

namespace IGCSClient.Controls
{
	/// <summary>
	/// Bool editor
	/// </summary>
	public partial class BoolInputWPF : UserControl, IInputControl<bool>
	{
        private bool _suppressValueChangedEvent = false;
        public event EventHandler ValueChanged;

		public BoolInputWPF()
		{
			InitializeComponent();
		}

        public void SetValueSilently(bool value)
        {
            _suppressValueChangedEvent = true;
            _checkboxControl.IsChecked = value;
            _suppressValueChangedEvent = false;
        }

        public static readonly DependencyProperty LabelFontSizeProperty =
    DependencyProperty.Register("LabelFontSize", typeof(double), typeof(BoolInputWPF), new PropertyMetadata(12.0));

        private void _checkboxControl_Checked(object sender, EventArgs e)
		{
            if (!_suppressValueChangedEvent)
            {
                this.ValueChanged.RaiseEvent(this);
            }
        }


		private void _checkboxControl_OnUnchecked(object sender, RoutedEventArgs e)
		{
            if (!_suppressValueChangedEvent)
            {
                this.ValueChanged.RaiseEvent(this);
            }
        }


		public void SetValueFromString(string valueAsString, bool defaultValue)
		{
			if(!Boolean.TryParse(valueAsString, out var valueToSet))
			{
				valueToSet = defaultValue;
			}
			this.Value = valueToSet;
		}
		

		#region Properties
		/// <inheritdoc/>
		public bool Value
		{
			get { return _checkboxControl.IsChecked.HasValue && _checkboxControl.IsChecked.Value; }
			set { _checkboxControl.IsChecked = value; }
		}

        //public string Header
        //{
        //	get { return _headerControl.Header?.ToString() ?? string.Empty; }
        //	set { _headerControl.Header = value; }
        //}

        public string Label
		{
			get { return _labelControl.Content?.ToString() ?? string.Empty; }
			set { _labelControl.Content = value; }
		}

        public double LabelFontSize
        {
            get { return (double)GetValue(LabelFontSizeProperty); }
            set { SetValue(LabelFontSizeProperty, value); }
        }
        #endregion
    }
}
