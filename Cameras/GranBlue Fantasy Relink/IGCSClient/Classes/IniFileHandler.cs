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

using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Simple ini file handler. From: https://stackoverflow.com/a/14906422
	/// </summary>
	public class IniFileHandler
	{
		private string _path;
		private string _executableName = Assembly.GetExecutingAssembly().GetName().Name;

		[DllImport("kernel32", CharSet = CharSet.Unicode)]
		static extern long WritePrivateProfileString(string Section, string Key, string Value, string FilePath);
		[DllImport("kernel32", CharSet = CharSet.Unicode)]
		static extern int GetPrivateProfileString(string Section, string Key, string Default, StringBuilder RetVal, int Size, string FilePath);


		public IniFileHandler(string iniFilename = null)
		{
			_path = Path.Combine(Path.GetDirectoryName(Application.ExecutablePath) ?? string.Empty, iniFilename);
		}


		public bool FileExists()
		{
			return File.Exists(_path);
		}

		public string Read(string key, string section = null)
		{
			var retVal = new StringBuilder(1024);
			GetPrivateProfileString(section ?? _executableName, key, "", retVal, 1024, _path);
			return retVal.ToString();
		}

		public void Write(string key, string value, string section = null)
		{
			WritePrivateProfileString(section ?? _executableName, key, value, _path);
		}

		public void DeleteKey(string key, string section = null)
		{
			Write(key, null, section ?? _executableName);
		}

		public void DeleteSection(string section = null)
		{
			Write(null, null, section ?? _executableName);
		}

		public bool KeyExists(string key, string section = null)
		{
			return Read(key, section).Length > 0;
		}
	}
}
