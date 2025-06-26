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
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Class which injects a dll in a given process.
	/// </summary>
    internal class DllInjector
    {
		/// <summary>
		/// Injects the dll with the path specified into the process with the id specified. 
		/// </summary>
		/// <param name="processId">the PID of the process to inject the dll into.</param>
		/// <param name="dllPathNameToInject">The full path + filename of the dll to inject</param>
		/// <returns>true if succeeded, false otherwise. If false is returned, <see cref="LastError"/> is set with the error code.</returns>
		public bool PerformInjection(int processId, string dllPathNameToInject)
		{
			this.LastError = 0;

			uint dllLengthToPassInBytes = (uint)((dllPathNameToInject.Length + 1) * Marshal.SizeOf(typeof(char)));

			this.LastActionPerformed = "Opening the host process";
			LogHandlerSingleton.Instance().LogLine("Opening the host process", "DllInjector", true);
			IntPtr processHandle = Win32Wrapper.OpenProcess(ProcessAccessFlags.CreateThread | ProcessAccessFlags.QueryInformation | ProcessAccessFlags.VirtualMemoryOperation |
															ProcessAccessFlags.VirtualMemoryWrite | ProcessAccessFlags.VirtualMemoryRead, 
															false, (uint)processId);
			if(processHandle==IntPtr.Zero)
			{
				// failed, so set the error code and return
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}

			this.LastActionPerformed = "Obtaining the address of LoadLibraryA";
			LogHandlerSingleton.Instance().LogLine("Obtaining the address of LoadLibraryA", "DllInjector", true);
			IntPtr loadLibraryAddress = Win32Wrapper.GetProcAddress(Win32Wrapper.GetModuleHandle("kernel32.dll"), "LoadLibraryA");
			if(loadLibraryAddress==IntPtr.Zero)
			{
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}

			this.LastActionPerformed = "Allocating memory in the host process for the dll filename";
			LogHandlerSingleton.Instance().LogLine("Allocating memory in the host process for the dll filename", "DllInjector", true);
			IntPtr memoryInTargetProcess = Win32Wrapper.VirtualAllocEx(processHandle, IntPtr.Zero, dllLengthToPassInBytes, AllocationType.Commit | AllocationType.Reserve, 
																	   MemoryProtection.ReadWrite);
			if(memoryInTargetProcess==IntPtr.Zero)
			{
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}

			Thread.Sleep(500);

			this.LastActionPerformed = "Writing dll filename into memory allocated in host process";
			LogHandlerSingleton.Instance().LogLine("Writing dll filename into memory allocated in host process", "DllInjector", true);
			var bytesToWrite = Encoding.Default.GetBytes(dllPathNameToInject);
			bool result = Win32Wrapper.WriteProcessMemory(processHandle, memoryInTargetProcess, bytesToWrite, dllLengthToPassInBytes, out var bytesWritten);
			if(!result || (bytesWritten.ToInt32()!=bytesToWrite.Length+1))
			{
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}

			this.LastActionPerformed = "Creating a thread in the host process to load the dll";
			LogHandlerSingleton.Instance().LogLine("Creating a thread in the host process to load the dll", "DllInjector", true);
			IntPtr remoteThreadHandle = Win32Wrapper.CreateRemoteThread(processHandle, IntPtr.Zero, 0, loadLibraryAddress, memoryInTargetProcess, 0, IntPtr.Zero);
			if(remoteThreadHandle==IntPtr.Zero)
			{
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}
			// no clean up of the memory, we're not going to 'unload' the dll... 
			result = Win32Wrapper.CloseHandle(processHandle);
			if(!result)
			{
				this.LastError = Marshal.GetLastWin32Error();
				return false;
			}

			this.LastActionPerformed = "Done";
			LogHandlerSingleton.Instance().LogLine("Injection completed", "DllInjector", true);
			return true;
		}

        public bool PerformInjectionWithD3DDisabled(int processId, string dllPathNameToInject)
        {
            this.LastError = 0;

            // Create a unique shared memory name for this process
            string sharedMemName = $"IGCS_D3D_DISABLED_{processId}";

            // Create the shared memory before injection
            bool createdOk = CreateSharedMemoryFlag(sharedMemName);
            if (!createdOk)
            {
                this.LastError = Marshal.GetLastWin32Error();
                this.LastActionPerformed = "Failed to create shared memory flag";
                return false;
            }

            // Now perform the normal injection
            LogHandlerSingleton.Instance().LogLine($"Injecting with D3D hooking disabled via shared memory ({sharedMemName})", "DllInjector", true);
            return PerformInjection(processId, dllPathNameToInject);
        }

        private bool CreateSharedMemoryFlag(string memName)
        {
            try
            {
                // Use the Win32 API to create named shared memory that the DLL can check for
                // The string needs to match the wide character format in the DLL
                string wideMemName = memName; // C# strings are already Unicode

                IntPtr hMapFile = Win32Wrapper.CreateFileMapping(
                    new IntPtr(-1),                  // Use paging file
                    IntPtr.Zero,                     // Default security
                    0x04,                            // PAGE_READWRITE
                    0,                               // Maximum object size (high-order DWORD)
                    4,                               // Maximum object size (low-order DWORD)
                    wideMemName);                    // Name of mapping object

                if (hMapFile == IntPtr.Zero)
                {
                    LogHandlerSingleton.Instance().LogLine($"Could not create file mapping object ({Marshal.GetLastWin32Error()})", "DllInjector", true);
                    return false;
                }

                // Map view of the file
                IntPtr pBuf = Win32Wrapper.MapViewOfFile(
                    hMapFile,                        // Handle to map object
                    0xF001F,                         // FILE_MAP_ALL_ACCESS
                    0,                               // High-order DWORD of file offset
                    0,                               // Low-order DWORD of file offset
                    4);                              // Number of bytes to map

                if (pBuf == IntPtr.Zero)
                {
                    LogHandlerSingleton.Instance().LogLine($"Could not map view of file ({Marshal.GetLastWin32Error()})", "DllInjector", true);
                    Win32Wrapper.CloseHandle(hMapFile);
                    return false;
                }

                // Write a flag value to the shared memory
                Marshal.WriteInt32(pBuf, 1);  // Value 1 indicates D3D hooking should be disabled

                // We intentionally don't unmap or close handles - they'll be automatically cleaned up
                // when the process exits, and we want the DLL to be able to access them.

                return true;
            }
            catch (Exception ex)
            {
                LogHandlerSingleton.Instance().LogLine($"Error creating shared memory: {ex.Message}", "DllInjector", true);
                return false;
            }
        }

        #region Properties
        /// <summary>
        /// If PerformInjection returns false, this property contains the last error code returned by GetLastError()
        /// </summary>
        public int LastError {get;set;}
		/// <summary>
		/// The last action performed by the injector. Needed for error reporting.
		/// </summary>
		public string LastActionPerformed {get;set;}
		#endregion
	}
}
