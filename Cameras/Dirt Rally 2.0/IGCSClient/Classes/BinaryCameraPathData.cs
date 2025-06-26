using System;
using System.Runtime.InteropServices;

namespace IGCSClient.Models
{
    // These structs must match the C++ structures exactly

    // Binary format version
    public static class BinaryPathFormat
    {
        public const byte VERSION = 1;
    }

    // Header for the entire data packet
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct BinaryPathHeader
    {
        public byte FormatVersion;
        public byte PathCount;
    }

    // Header for each path
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct BinaryPathData
    {
        public ushort NameLength;
        public byte NodeCount;
        // Followed by nameLength bytes for the path name
        // Followed by nodeCount BinaryNodeData structures
    }

    // Data for a single camera node
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct BinaryNodeData
    {
        public byte Index;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] Position;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] Rotation;

        public float FOV;
    }

    // Helper class for binary operations
    public static class BinaryUtils
    {
        // Convert byte array to struct
        public static T BytesToStruct<T>(byte[] bytes, int offset) where T : struct
        {
            int size = Marshal.SizeOf<T>();
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                Marshal.Copy(bytes, offset, ptr, size);
                return Marshal.PtrToStructure<T>(ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        // Read string from byte array
        public static string ReadString(byte[] bytes, int offset, int length)
        {
            return System.Text.Encoding.ASCII.GetString(bytes, offset, length);
        }
    }
}
