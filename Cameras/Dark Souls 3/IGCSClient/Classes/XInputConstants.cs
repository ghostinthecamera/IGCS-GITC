using System.Collections.Generic;

namespace IGCSClient.Classes
{
    /// <summary>
    /// Constants for XInput gamepad buttons with mapping to byte values matching the DLL side
    /// </summary>
    public static class XInputConstants
    {
        // Button flag constants
        public const int A = 0x1000;
        public const int B = 0x2000;
        public const int X = 0x4000;
        public const int Y = 0x8000;
        public const int DPAD_UP = 0x0001;
        public const int DPAD_DOWN = 0x0002;
        public const int DPAD_LEFT = 0x0004;
        public const int DPAD_RIGHT = 0x0008;
        public const int LEFT_SHOULDER = 0x0100;
        public const int RIGHT_SHOULDER = 0x0200;
        public const int L3 = 0x0040;
        public const int R3 = 0x0080;
        public const int START = 0x0010;
        public const int BACK = 0x0020;

        // Mapping that matches the DLL's kIdToXInputTable exactly
        public static readonly Dictionary<int, byte> ButtonToByteMap = new Dictionary<int, byte>()
        {
            { A, 1 },
            { B, 2 },
            { X, 3 },
            { Y, 4 },
            { DPAD_UP, 5 },
            { DPAD_DOWN, 6 },
            { DPAD_LEFT, 7 },
            { DPAD_RIGHT, 8 },
            { LEFT_SHOULDER, 9 },
            { RIGHT_SHOULDER, 10 },
            { L3, 11 },
            { R3, 12 },
            { START, 13 },
            { BACK, 14 }
        };

        // Reverse mapping
        public static readonly Dictionary<byte, int> ByteToButtonMap = new Dictionary<byte, int>()
        {
            { 1, A },
            { 2, B },
            { 3, X },
            { 4, Y },
            { 5, DPAD_UP },
            { 6, DPAD_DOWN },
            { 7, DPAD_LEFT },
            { 8, DPAD_RIGHT },
            { 9, LEFT_SHOULDER },
            { 10, RIGHT_SHOULDER },
            { 11, L3 },
            { 12, R3 },
            { 13, START },
            { 14, BACK }
        };
    }
}
