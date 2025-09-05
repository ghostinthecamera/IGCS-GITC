using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace IGCSClient.Classes
{
    /// <summary>
    /// Handles gamepad input detection for key binding
    /// </summary>
    public class GamepadInputHandler
    {
        // XInput structures and API
        [StructLayout(LayoutKind.Sequential)]
        private struct XINPUT_STATE
        {
            public uint dwPacketNumber;
            public XINPUT_GAMEPAD Gamepad;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct XINPUT_GAMEPAD
        {
            public ushort wButtons;
            public byte bLeftTrigger;
            public byte bRightTrigger;
            public short sThumbLX;
            public short sThumbLY;
            public short sThumbRX;
            public short sThumbRY;
        }

        [DllImport("xinput1_4.dll", CallingConvention = CallingConvention.StdCall)]
        private static extern uint XInputGetState(uint dwUserIndex, ref XINPUT_STATE pState);

        private const uint ERROR_SUCCESS = 0;
        private static bool _isListening;
        private static Thread _listenerThread;
        private static ushort _lastButtons;
        private static bool _lb;
        private static bool _rb;
        private static bool _leftThumb;

        public static event EventHandler<GamepadButtonEventArgs> ButtonPressed;

        /// <summary>
        /// Starts listening for gamepad input
        /// </summary>
        public static void StartListening()
        {
            if (_isListening)
                return;

            _isListening = true;
            _listenerThread = new Thread(ListenerThreadMethod)
            {
                IsBackground = true,
                Name = "Gamepad Input Listener"
            };
            _listenerThread.Start();
        }

        /// <summary>
        /// Stops listening for gamepad input
        /// </summary>
        public static void StopListening()
        {
            _isListening = false;
            _listenerThread?.Join(500);
            _listenerThread = null;
        }

        private static void ListenerThreadMethod()
        {
            var state = new XINPUT_STATE();

            while (_isListening)
            {
                // Check for controller input on controller 0
                if (XInputGetState(0, ref state) == ERROR_SUCCESS)
                {
                    var buttons = state.Gamepad.wButtons;

                    // If buttons changed
                    if (buttons != _lastButtons)
                    {
                        // Check for "modifier" buttons (LB, RB, Left Thumb)
                        _lb = (buttons & XInputConstants.LEFT_SHOULDER) != 0;
                        _rb = (buttons & XInputConstants.RIGHT_SHOULDER) != 0;
                        //_leftThumb = (buttons & XInputConstants.L3) != 0;

                        // Find newly pressed buttons
                        var pressed = (ushort)(buttons & ~_lastButtons);

                        if (pressed != 0)
                        {
                            // Find the individual button that was pressed
                            foreach (var buttonFlag in GetIndividualButtonFlags().Where(buttonFlag => (pressed & buttonFlag) != 0))
                            {
                                // Raise event for this button press
                                OnButtonPressed(buttonFlag, _lb, _rb, _leftThumb);
                                break;  // Only handle one button at a time for binding
                            }
                        }

                        _lastButtons = buttons;
                    }
                }

                Thread.Sleep(50);  // Poll every 50ms
            }
        }

        private static List<int> GetIndividualButtonFlags()
        {
            return new List<int>
            {
                XInputConstants.A,
                XInputConstants.B,
                XInputConstants.X,
                XInputConstants.Y,
                XInputConstants.DPAD_UP,
                XInputConstants.DPAD_DOWN,
                XInputConstants.DPAD_LEFT,
                XInputConstants.DPAD_RIGHT,
                XInputConstants.START,
                XInputConstants.BACK,
                XInputConstants.L3,
                XInputConstants.R3,
                XInputConstants.LEFT_SHOULDER,
                XInputConstants.RIGHT_SHOULDER
            };
        }

        private static void OnButtonPressed(int buttonCode, bool lb, bool rb, bool leftThumb)
        {
            ButtonPressed?.Invoke(null, new GamepadButtonEventArgs(buttonCode, lb, rb, leftThumb));
        }
    }

    public class GamepadButtonEventArgs : EventArgs
    {
        public int ButtonCode { get; }
        public bool LB { get; }
        public bool RB { get; }
        public bool LeftThumb { get; }

        public GamepadButtonEventArgs(int buttonCode, bool lb, bool rb, bool leftThumb)
        {
            ButtonCode = buttonCode;
            LB = lb;
            RB = rb;
            LeftThumb = leftThumb;
        }
    }
}