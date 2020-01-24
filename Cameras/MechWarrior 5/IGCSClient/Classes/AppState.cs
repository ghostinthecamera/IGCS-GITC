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
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using IGCSClient.Interfaces;
using ModernWpf;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Singleton class containing the single appstate instance which contains the state of this application at runtime. 
	/// </summary>
	public static class AppStateSingleton
	{
		private static AppState _instance = new AppState();

		/// <summary>Dummy static constructor to make sure threadsafe initialization is performed.</summary>
		static AppStateSingleton() {}

		/// <summary>
		/// Gets the single instance in use in this application
		/// </summary>
		/// <returns></returns>
		public static AppState Instance() => _instance;
	}

	/// <summary>
	/// The class containing the actual application state at runtime.
	/// </summary>
	public class AppState
	{
		#region Members
		private readonly List<ISetting> _settings;
		private readonly List<KeyBindingSetting> _keyBindings;
		private Dictionary<string, DllCacheData> _recentProcessesWithDllsUsed;		// key: process name (blabla.exe), value: dll name (full path) & DateTime last used.
		private Process _attachedProcess;
		private IntPtr _attachedProcessMainWindowHwnd;
		private List<Resolution> _recentlyUsedResolutions;
		#endregion


		internal AppState()
		{
			_settings = new List<ISetting>();
			_keyBindings = new List<KeyBindingSetting>();
			_recentProcessesWithDllsUsed = new Dictionary<string, DllCacheData>();
			_attachedProcessMainWindowHwnd = IntPtr.Zero;
			_recentlyUsedResolutions = new List<Resolution>();
		}


		public IntPtr GetMainWindowHandleOfAttachedProcess()
		{
			if(this.AttachedProcess == null)
			{
				return IntPtr.Zero;
			}

			var screenOfWindow = Screen.FromHandle(_attachedProcessMainWindowHwnd);
			if(screenOfWindow.Bounds.IsEmpty)
			{
				// obtain it using window traversal and top most 
				var allWindowHandles = GeneralUtils.GetProcessWindowHandles(_attachedProcess);
				foreach(var hwnd in allWindowHandles)
				{
					WINDOWINFO info = new WINDOWINFO();
					Win32Wrapper.GetWindowInfo(hwnd, ref info);
					screenOfWindow = Screen.FromHandle(hwnd);
					if(!screenOfWindow.Bounds.IsEmpty && info.rcWindow.Width>10)
					{
						// window is on a monitor and has a width wider than 10
						_attachedProcessMainWindowHwnd = hwnd;
						break;
					}
				}
			}
			return _attachedProcessMainWindowHwnd;
		}


		public void AddSetting(ISetting settingToAdd)
		{
			_settings.Add(settingToAdd);
		}


		public void AddKeyBinding(KeyBindingSetting bindingToAdd)
		{
			_keyBindings.Add(bindingToAdd);
		}


		public void SendInitialData()
		{
			SendSettings();
			SendKeyBindings();
		}


		public void SendSettings()
		{
			foreach(var s in _settings)
			{
				s.SendValueAsMessage();
			}
		}


		public void SendKeyBindings()
		{
			foreach(var kb in _keyBindings)
			{
				kb.SendValueAsMessage();
			}
		}
		

		public void LoadRecentProcessList()
		{
			try
			{
				var fileName = Path.Combine(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), ConstantsEnums.IGCSSettingsFolder),
											ConstantsEnums.RecentlyUsedFilename);
				if(!File.Exists(fileName))
				{
					return;
				}

				// format: 
				// processname1|dllname1|Ticks
				// processname2|dllname1|Ticks
				// processname3|dllname2|Ticks
				// ...
				// example:
				// mygame.exe|..\..\mygamecameratools.dll|2384763243
				// mygame2.exe|unlocker.dll|23462112323
				var allLines = File.ReadAllLines(fileName);
				foreach(var line in allLines)
				{
					var parts = line.Split('|');
					if(parts.Length != 3)
					{
						continue;
					}

					_recentProcessesWithDllsUsed[parts[0]] = new DllCacheData(parts[1], new DateTime(Convert.ToInt64(parts[2])));
				}
			}
			catch
			{
				// ignore, as we can't do much about it anyway... 
			}
		}
		

		public void SaveRecentProcessList()
		{
			if(_recentProcessesWithDllsUsed.Count<=0)
			{
				return;
			}
			// trim list if there are more than the max
			if(_recentProcessesWithDllsUsed.Count > ConstantsEnums.NumberOfDllCacheEntriesToKeep)
			{
				// keep the most recent 100. 
				var mostRecent = _recentProcessesWithDllsUsed.OrderByDescending(kvp=>kvp.Value.LastUsedDate).Take(ConstantsEnums.NumberOfDllCacheEntriesToKeep).Select(kvp=>kvp.Key).ToList();
				// remove the rest. 
				var toRemove = _recentProcessesWithDllsUsed.Keys.Except(mostRecent).ToList();
				foreach(var key in toRemove)
				{
					_recentProcessesWithDllsUsed.Remove(key);
				}
			}
			var folderName = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), ConstantsEnums.IGCSSettingsFolder);
			if(!Directory.Exists(folderName))
			{
				Directory.CreateDirectory(folderName);
			}
			var fileName = Path.Combine(folderName, ConstantsEnums.RecentlyUsedFilename);
			var linesToWrite = _recentProcessesWithDllsUsed.Select(kvp=>kvp.Key + "|" + kvp.Value.DllName + "|" + kvp.Value.LastUsedDate.Ticks);
			File.WriteAllLines(fileName, linesToWrite);
		}


		public void SaveSettingsToIni()
		{
			var iniFile = new IniFileHandler(ConstantsEnums.IniFilename);
			foreach(var setting in _settings)
			{
				iniFile.Write(setting.Name, setting.GetValueAsString(), "CameraSettings");
			}

			foreach(ISetting binding in _keyBindings)
			{
				iniFile.Write(binding.Name, binding.GetValueAsString(), "KeyBindings");
			}

			// other settings
			ApplicationTheme? currentTheme = ThemeManager.Current.ApplicationTheme;
			int themeID = -1;	// not defined, so use default.
			if(currentTheme != null)
			{
				themeID = (int)currentTheme.Value;
			}
			iniFile.Write("ThemeID", themeID.ToString(), "MiscSettings");
			System.Windows.Media.Color? accentColor = ThemeManager.Current.AccentColor;
			if(accentColor == null)
			{
				iniFile.DeleteKey("AccentColor", "MiscSettings");
			}
			else
			{
				iniFile.Write("AccentColor", accentColor.ToString(), "MiscSettings");
			}

			int counter = 0;
			foreach(var r in _recentlyUsedResolutions)
			{
				// Format: resolution|aspect ratio
				// resulution: widthxheight
				// aspect ratio: width:height
				iniFile.Write("Resolution" + counter, r.ToStringForIniFile(), "RecentlyUsedResolutions");
				counter++;
			}
		}


		public void LoadFromIni()
		{
			var iniFile = new IniFileHandler(ConstantsEnums.IniFilename);
			if(iniFile.FileExists())
			{
				foreach(var setting in _settings)
				{
					setting.SetValueFromString(iniFile.Read(setting.Name, "CameraSettings"));
				}

				foreach(var binding in _keyBindings)
				{
					binding.SetValueFromString(iniFile.Read(binding.Name, "KeyBindings"));
				}

				// other settings
				var themeID = Convert.ToInt32(iniFile.Read("ThemeID", "MiscSettings"));
				if(themeID >= 0)
				{
					ThemeManager.Current.ApplicationTheme = (ApplicationTheme)themeID;
				}

				var accentColor = iniFile.Read("AccentColor", "MiscSettings");
				if(!string.IsNullOrWhiteSpace(accentColor))
				{
					ThemeManager.Current.AccentColor = (System.Windows.Media.Color)System.Windows.Media.ColorConverter.ConvertFromString(accentColor);
				}

				for(int i = 0; i < ConstantsEnums.NumberOfResolutionsToKeep; i++)
				{
					string keyName = "Resolution" + i;
					if(!iniFile.KeyExists(keyName, "RecentlyUsedResolutions"))
					{
						break;
					}

					var resolutionAsString = iniFile.Read(keyName, "RecentlyUsedResolutions");
					if(!string.IsNullOrEmpty(resolutionAsString))
					{
						_recentlyUsedResolutions.Add(Resolution.FromString(resolutionAsString));
					}
				}
			}
		}


		public List<string> GetRecentProcessesList()
		{
			return _recentProcessesWithDllsUsed.Keys.ToList();
		}


		public string GetDllNameForSelectedProcess(string processName)
		{
			string toReturn = string.Empty;
			if(_recentProcessesWithDllsUsed.TryGetValue(processName, out DllCacheData dllData))
			{
				toReturn = dllData.DllName;
			}
			return toReturn;
		}


		public void AddDllNameForProcess(string processName, string dllName)
		{
			_recentProcessesWithDllsUsed[processName] = new DllCacheData(dllName, DateTime.Now);
		}


		public void SetAttachedProcess(Process attachedProcess)
		{
			_attachedProcess = attachedProcess;
			_attachedProcessMainWindowHwnd = attachedProcess.MainWindowHandle;
		}


		public void StoreRecentlyUsedResolutions(IEnumerable<Resolution> recentlyUsedResolutions)
		{
			if(recentlyUsedResolutions == null)
			{
				return;
			}
			_recentlyUsedResolutions.Clear();
			_recentlyUsedResolutions.AddRange(recentlyUsedResolutions);
		}


		#region Properties
		public ISetting[] Settings
		{
			get { return _settings.ToArray(); }
		}

		public ISetting[] KeyBindings
		{
			get { return _keyBindings.Cast<ISetting>().ToArray(); }
		}

		public Process AttachedProcess
		{
			get { return _attachedProcess; }
		}

		public List<Resolution> RecentlyUsedResolutions
		{
			get { return _recentlyUsedResolutions; }
		}
		#endregion
	}
}
