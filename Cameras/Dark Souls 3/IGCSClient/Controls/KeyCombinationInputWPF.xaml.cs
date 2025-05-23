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
using System.Windows.Input;
using IGCSClient.Classes;
using IGCSClient.Interfaces;

namespace IGCSClient.Controls
{
	/// <summary>
	/// Key combination editor
	/// </summary>
	public partial class KeyCombinationInputWPF : UserControl, IInputControl<KeyCombination>
	{
		#region Members
		private KeyCombination _tmpCombination, _toEdit, _activeCombination;
		private bool _editing;
        private InputType _lastInputType = InputType.Keyboard;

        public event EventHandler ValueChanged;
        #endregion

        private enum InputType
        {
            Keyboard,
            Gamepad
        }

        public KeyCombinationInputWPF()
		{
			InitializeComponent();
			_tmpCombination = new KeyCombination();
			_okButton.Visibility = Visibility.Hidden;
			_cancelButton.Visibility = Visibility.Hidden;
            GamepadInputHandler.ButtonPressed += GamepadInputHandler_ButtonPressed;
        }

        private void GamepadInputHandler_ButtonPressed(object sender, GamepadButtonEventArgs e)
        {
            if (!_editing)
                return;

            // Create gamepad key combination
            _tmpCombination.SetNewValues(e.RB, e.LB, false, e.ButtonCode, true);
            _lastInputType = InputType.Gamepad;

            // Update UI on the UI thread
            Dispatcher.Invoke(() =>
            {
                ReflectEditingState();
                _inputTypeIndicator.Text = "Gamepad";
                _inputTypeIndicator.Foreground = System.Windows.Media.Brushes.Green;
            });
        }


        public void SetValueFromString(string valueAsString, KeyCombination defaultValue)
        {
            if (_toEdit == null)
            {
                _toEdit = new KeyCombination();
                _activeCombination = _toEdit;
            }

            if (!_toEdit.SetValueFromString(valueAsString))
            {
                _toEdit.CopyValues(defaultValue);
            }

            // Update input type indicator based on the loaded binding
            _lastInputType = _toEdit.IsGamepadButton ? InputType.Gamepad : InputType.Keyboard;
            UpdateInputTypeIndicator();

            ReflectEditingState();
        }

        private void UpdateInputTypeIndicator()
        {
            _inputTypeIndicator.Text = _lastInputType == InputType.Gamepad ? "Gamepad" : "Keyboard";
            if (_lastInputType == InputType.Gamepad)
            {
                _inputTypeIndicator.Foreground = System.Windows.Media.Brushes.Green;
            }
            else
            {
                // Reset to default theme text color for Keyboard
                _inputTypeIndicator.ClearValue(TextBlock.ForegroundProperty);
            }
        }

        public void Setup(KeyCombination initialCombination)
		{
			ArgumentVerifier.CantBeNull(initialCombination, nameof(initialCombination));
			_toEdit = initialCombination;
			_activeCombination = _toEdit;

            _lastInputType = _toEdit.IsGamepadButton ? InputType.Gamepad : InputType.Keyboard;
            UpdateInputTypeIndicator();

            ReflectEditingState();
		}


		private void _keyInputTextBox_OnPreviewKeyDown(object sender, KeyEventArgs e)
		{
			if(!_editing)
			{
				return;
			}

			if(e.Key == Key.Escape)
			{
				EndEditing();
				return;
			}

			var altPressed = (e.KeyboardDevice.Modifiers & ModifierKeys.Alt)!=0;
			var ctrlPressed = (e.KeyboardDevice.Modifiers & ModifierKeys.Control)!=0;
			var shiftPressed = (e.KeyboardDevice.Modifiers & ModifierKeys.Shift)!=0;

			// Convert from WPF key to a virtual win32 keycode, which is what we're after. 
			var keyCode = (int)KeyInterop.VirtualKeyFromKey(e.Key);
			
			if(keyCode > 255 ||
			   e.Key == Key.LeftCtrl ||
			   e.Key == Key.RightCtrl ||
			   e.Key == Key.LeftAlt ||
			   e.Key == Key.RightAlt ||
			   e.Key == Key.LeftShift ||
			   e.Key == Key.RightShift ||
			   e.Key == Key.LWin ||
			   e.Key == Key.RWin ||
			   e.Key == Key.Clear ||
			   e.Key == Key.OemClear ||
			   e.Key == Key.Apps)
			{
				// ignore
				return;
			}

			_tmpCombination.SetNewValues(altPressed, ctrlPressed, shiftPressed, keyCode, false);
            _lastInputType = InputType.Keyboard;
            _inputTypeIndicator.Text = "Keyboard";
            _inputTypeIndicator.ClearValue(TextBlock.ForegroundProperty);

            ReflectEditingState();
			e.Handled = true;
		}
		
		
		private void SwitchToEditModeIfNeeded()
		{
			if(_editing)
			{
				return;
			}

			StartEditing();
		}


		private void StartEditing()
		{
			_editing = true;
			_tmpCombination.Clear();
			_activeCombination = _tmpCombination;
			_cancelButton.Visibility = Visibility.Visible;
			_okButton.Visibility = Visibility.Visible;

            // Start listening for gamepad input automatically
            GamepadInputHandler.StartListening();

            // Show input type indicator
            _inputTypePanel.Visibility = Visibility.Visible;

            ReflectEditingState();
		}


		private void EndEditing()
		{
			if(!_editing)
			{
				return;
			}
			_activeCombination = _toEdit;
			_cancelButton.Visibility = Visibility.Hidden;
			_okButton.Visibility = Visibility.Hidden;

            _inputTypePanel.Visibility = Visibility.Collapsed;

            _editing = false;
			ReflectEditingState();
			this.ValueChanged.RaiseEvent(this);
		}


		private void ReflectEditingState()
		{
			if(_activeCombination == null)
			{
				return;
			}
			_keyInputTextBox.Text = _activeCombination.ToString();
		}


		private void _okButton_OnClick(object sender, RoutedEventArgs e)
		{
			_toEdit.CopyValues(_tmpCombination);
			EndEditing();
		}


		private void _cancelButton_OnClick(object sender, RoutedEventArgs e)
		{
			EndEditing();
		}


		private void _keyInputTextBox_OnGotFocus(object sender, RoutedEventArgs e)
		{
			SwitchToEditModeIfNeeded();
		}


        #region Properties
        public KeyCombination Value
        {
            get => _toEdit;
            set
            {
                _toEdit = value;
                ReflectEditingState();
            }
        }

        public string Header
        {
            get => _headerTextBlock.Text;
            set => _headerTextBlock.Text = value;
        }

        private InputType LastInputType => _lastInputType;
        #endregion
    }
}
