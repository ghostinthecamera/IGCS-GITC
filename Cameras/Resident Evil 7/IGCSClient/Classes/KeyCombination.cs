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
using System.Linq;
using System.Text;
using Windows.AI.MachineLearning;

namespace IGCSClient.Classes
{
    /// <summary>
    /// Simple class which represents a keycombination, like CTRL-S
    /// </summary>
    public class KeyCombination
    {
        #region Members
        private string _textualRepresentation;
        private bool _altPressed;
        private bool _ctrlPressed;
        private bool _shiftPressed;
        private int _keyCode;
        private bool _isGamepadButton;
        private static List<string> KeyCodeToStringLookup = new List<string>()
        {
			// 0x00 - 0x0F
			"", "", "", "Cancel", "", "", "", "", "Backspace", "Tab", "", "", "Clear", "Enter", "", "",
			// 0x10 - 0x1F
			"Shift", "Control", "Alt", "Pause", "Caps Lock", "", "", "", "", "", "", "Escape", "", "", "", "",
			// 0x20 - 0x2F
			"Space", "Page Up", "Page Down", "End", "Home", "Left Arrow", "Up Arrow", "Right Arrow", "Down Arrow",
            "Select", "", "", "Print Screen", "Insert", "Delete", "Help",
			// 0x30 - 0x3F
			"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "", "", "", "", "", "",
			// 0x40 - 0x4F
			"", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
			// 0x50 - 0x5F
			"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows", "Right Windows", "", "", "Sleep",
			// 0x60 - 0x6F
			"Numpad 0", "Numpad 1", "Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7", "Numpad 8", "Numpad 9",
            "Numpad *", "Numpad +", "", "Numpad -", "Numpad Decimal", "Numpad /",
			// 0x70 - 0x7F
			"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16",
			// 0x80 - 0x8F
			"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "", "", "", "", "", "", "", "",
			// 0x90 - 0x9F
			"Num Lock", "Scroll Lock", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
			// 0xA0 - 0xAF
			"Left Shift", "Right Shift", "Left Control", "Right Control", "Left Alt", "Right Alt", "", "", "", "", "", "", "", "", "", "",
            // 0xB0 - 0xBF
            "", "", "", "", "", "", "", "", "", "", // 0xB0-0xB9
            ";", // 0xBA (VK_OEM_1) - Semicolon
            "=", // 0xBB (VK_OEM_PLUS) - Equals
            ",", // 0xBC (VK_OEM_COMMA) - Comma
            "-", // 0xBD (VK_OEM_MINUS) - Hyphen
            ".", // 0xBE (VK_OEM_PERIOD) - Period
            "/", // 0xBF (VK_OEM_2) - Forward Slash
            // 0xC0 - 0xCF
            "`", // 0xC0 (VK_OEM_3) - Backtick
            "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            // 0xD0 - 0xDF
            "", "", "", "", "", "", "", "", "", "", // 0xD0-0xD9
            "", // 0xDA
            "[", // 0xDB (VK_OEM_4) - Left Bracket
            "\\", // 0xDC (VK_OEM_5) - Backslash
            "]", // 0xDD (VK_OEM_6) - Right Bracket
            "'", // 0xDE (VK_OEM_7) - Quote
            "", "",
            // 0xE0 - 0xEF
            "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            // 0xF0 - 0xFF
            "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""
        };

        private static Dictionary<int, string> GamepadButtonToStringLookup = new Dictionary<int, string>()
        {
            { 0x1000, "A" },
            { 0x2000, "B" },
            { 0x4000, "X" },
            { 0x8000, "Y" },
            { 0x0001, "D-Pad Up" },
            { 0x0002, "D-Pad Down" },
            { 0x0004, "D-Pad Left" },
            { 0x0008, "D-Pad Right" },
            { 0x0100, "LB" },
            { 0x0200, "RB" },
            { 0x0040, "L3" },
            { 0x0080, "R3" },
            { 0x0010, "Start" },
            { 0x0020, "Back" }
        };

        private static Dictionary<int, int> GamepadButtonToCodedLookup = new Dictionary<int, int>()
        {
            { 0x1000, 1 },
            { 0x2000, 2 },
            { 0x4000, 3 },
            { 0x8000, 4 },
            { 0x0001, 5 },
            { 0x0002, 6 },
            { 0x0004, 7 },
            { 0x0008, 8 },
            { 0x0100, 9 },
            { 0x0200, 10 },
            { 0x0040, 11 },
            { 0x0080, 12 },
            { 0x0010, 13 },
            { 0x0020, 14 }
        };
        #endregion

        public KeyCombination()
        {
            _textualRepresentation = string.Empty;
            _altPressed = false;
            _ctrlPressed = false;
            _shiftPressed = false;
            _keyCode = 0;
            _isGamepadButton = false;
        }


        public KeyCombination(int keyCode, bool isGamepadButton=false) : this(false, false, false, keyCode, isGamepadButton)
        { }


        public KeyCombination(bool altPressed, bool ctrlPressed, bool shiftPressed, int keyCode, bool isGamepadButton = false)
        {
            SetNewValues(altPressed, ctrlPressed, shiftPressed, keyCode, isGamepadButton);
        }


        public void CopyValues(KeyCombination toCopyFrom)
        {
            SetNewValues(toCopyFrom._altPressed, toCopyFrom._ctrlPressed, toCopyFrom._shiftPressed, toCopyFrom._keyCode, toCopyFrom._isGamepadButton);
        }

        public void Clear()
        {
            SetNewValues(false, false, false, 0, false);
        }


        public void SetNewValues(bool altPressed, bool ctrlPressed, bool shiftPressed, int keyCode, bool isGamepadButton)
        {
            _altPressed = altPressed;
            _shiftPressed = shiftPressed;
            _ctrlPressed = ctrlPressed;
            _keyCode = keyCode;
            _isGamepadButton = isGamepadButton;

            CreateTextualRepresentation();
        }


        public override string ToString()
        {
            return _textualRepresentation;
        }


        /// <summary>
        /// Fills this object with the values determined from the string specified. 
        /// </summary>
        /// <param name="valueAsString">The string to obtain the values from. Should be in the format: keycode,altpressed,ctrlpressed,shiftpressed<br/>
        /// where keycode is an int, altpressed, ctrlpressed and shiftpressed are 'true' or 'false' strings.</param>
        /// <returns>true if string was parsed successfully, false otherwise</returns>
        public bool SetValueFromString(string valueAsString)
        {
            if (string.IsNullOrWhiteSpace(valueAsString))
            {
                return false;
            }
            var fragments = valueAsString.Split(',');
            if (fragments.Length != 4 && fragments.Length != 5)
            {
                return false;
            }

            try
            {
                _keyCode = Convert.ToInt32(fragments[0]);
                _altPressed = Convert.ToBoolean(fragments[1]);
                _ctrlPressed = Convert.ToBoolean(fragments[2]);
                _shiftPressed = Convert.ToBoolean(fragments[3]);

                // Only set isGamepadButton if it's in the new format
                _isGamepadButton = fragments.Length == 5 && Convert.ToBoolean(fragments[4]);

                CreateTextualRepresentation();
            }
            catch
            {
                // some error occurred during parsing, ignore the values and the exception as it's not really useful to handle it further. 
                return false;
            }
            return true;
        }


        /// <summary>
        /// Returns the value of this object as a string for persistence purposes. The value returned is compatible with the method SetValueFromString.
        /// </summary>
        /// <returns>The string representation of the values of this object. Format: keycode,altpressed,ctrlpressed,shiftpressed<br/>
        /// where keycode is an int, altpressed, ctrlpressed and shiftpressed are 'true' or 'false' strings.</returns>
        public string GetValueAsString()
        {
            return $"{_keyCode},{_altPressed},{_ctrlPressed},{_shiftPressed},{_isGamepadButton}";
        }


        /// <summary>
        /// Gets the value of this keybinding as a byte array for a message to send to the other side of the named pipe.
        /// </summary>
        /// <returns>
        /// Keybinding is 4 bytes: keycode | altpressed | ctrlpressed | shiftpressed
        /// </returns>
        public byte[] GetValueAsByteArray()
        {
            var toReturn = new byte[5]; // Increased size to include gamepad flag
            if (_isGamepadButton)
            {
                toReturn[0] = XInputConstants.ButtonToByteMap.TryGetValue(_keyCode, out var mappedValue) ? mappedValue : Convert.ToByte(_keyCode);
            }
            else
            {
                // For keyboard keys, preserve the original keyCode
                // Direct casting to byte will preserve at least the lower 8 bits
                toReturn[0] = Convert.ToByte(_keyCode);
            }

            try
            {
                toReturn[1] = Convert.ToByte(_altPressed);
                toReturn[2] = Convert.ToByte(_ctrlPressed);
                toReturn[3] = Convert.ToByte(_shiftPressed);
                toReturn[4] = Convert.ToByte(_isGamepadButton);
                return toReturn;
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }

        }


        private void CreateTextualRepresentation()
        {
            var builder = new StringBuilder(512);

            // For gamepad buttons, use special format
            if (_isGamepadButton)
            {
                if (_altPressed)
                {
                    builder.Append("RB+");
                }

                if (_ctrlPressed)
                {
                    builder.Append("LB+");
                }

                if (_keyCode > 0)
                {
                    builder.Append(GetTextualRepresentationOfGamepadButton());
                }
            }
            else  // Standard keyboard format
            {
                if (_altPressed)
                {
                    builder.Append("Alt+");
                }

                if (_ctrlPressed)
                {
                    builder.Append("Ctrl+");
                }

                if (_shiftPressed)
                {
                    builder.Append("Shift+");
                }

                if (_keyCode > 0)
                {
                    builder.Append(GetTextualRepresentationOfKeyCode());
                }
            }

            _textualRepresentation = builder.ToString();
        }


        private string GetTextualRepresentationOfKeyCode()
        {
            if (_keyCode is > 255 or < 0)
            {
                return string.Empty;
            }

            // Added bounds check - if the keyCode is beyond our lookup table's size
            if (_keyCode >= KeyCodeToStringLookup.Count)
            {
                return $"Key(0x{_keyCode:X2})";
            }

            var keyName = KeyCodeToStringLookup[_keyCode];
            return string.IsNullOrEmpty(keyName) ? $"Key(0x{_keyCode:X2})" : keyName;
        }

        private string GetTextualRepresentationOfGamepadButton()
        {
            return GamepadButtonToStringLookup.TryGetValue(_keyCode, out var buttonName) ? buttonName : $"Gamepad Button(0x{_keyCode:X4})";
        }

        #region Properties
        public bool IsGamepadButton
        {
            get => _isGamepadButton;
            set
            {
                _isGamepadButton = value;
                CreateTextualRepresentation();
            }
        }

        public int KeyCode => _keyCode;
        public bool AltPressed => _altPressed;
        public bool CtrlPressed => _ctrlPressed;
        public bool ShiftPressed => _shiftPressed;
        #endregion
    }
}