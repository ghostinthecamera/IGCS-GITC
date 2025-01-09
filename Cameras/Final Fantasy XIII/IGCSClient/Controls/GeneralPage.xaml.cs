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
using System.ComponentModel;
using System.Configuration;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Forms;
using IGCSClient.Classes;
using IGCSClient.Forms;
using IGCSClient.GameSpecific.Classes;
using MessageBox = System.Windows.MessageBox;
using UserControl = System.Windows.Controls.UserControl;

namespace IGCSClient.Controls
{
	/// <summary>
	/// Interaction logic for GeneralPage.xaml
	/// </summary>
	public partial class GeneralPage : UserControl
	{
		#region Members
		private Process _selectedProcess;
		private string _defaultProcessName;
		private string _defaultDllName;

		public event EventHandler DllInjected;
		public event EventHandler AttachedProcessExited;
		#endregion

		public GeneralPage()
		{
			InitializeComponent();
			_selectedProcess = null;
		}


		private void _selectProcessButton_Click(object sender, RoutedEventArgs e)
		{
			var processSelector = new ProcessSelectorWPF(AppStateSingleton.Instance().GetRecentProcessesList());
			processSelector.Owner = Window.GetWindow(this);
			processSelector.ShowDialog();
			if(!processSelector.DialogResult.HasValue || !processSelector.DialogResult.Value)
			{
				return;
			}
			_selectedProcess = processSelector.SelectedProcess;
			DisplayProcessInfoForInjection();
			// pre-select the dll if there's no dll selected
			if(string.IsNullOrEmpty(_dllFilenameTextBox.Text))
			{
				_dllFilenameTextBox.Text = AppStateSingleton.Instance().GetDllNameForSelectedProcess(_selectedProcess?.MainModule?.ModuleName) ?? string.Empty;
			}
		}


		private void _browseForDllButton_Click(object sender, RoutedEventArgs e)
		{
			var openDllToInjectDialog = new OpenFileDialog();
			openDllToInjectDialog.Multiselect = false;
			openDllToInjectDialog.Title = "Please select the dll to inject";
			openDllToInjectDialog.DefaultExt = "dll";
			openDllToInjectDialog.Filter = "Dll files|*.dll";
			openDllToInjectDialog.InitialDirectory = Environment.CurrentDirectory;
			var result = openDllToInjectDialog.ShowDialog();
			if(result==DialogResult.Cancel)
			{
				return;
			}
			_dllFilenameTextBox.Text = openDllToInjectDialog.FileName;
			_dllFilenameTextBox.ToolTip = openDllToInjectDialog.FileName;
		}


		private void GeneralPage_OnLoaded(object sender, RoutedEventArgs e)
		{
			LoadDefaultNamesFromConfigFile();
			FindDefaultProcess();
			DisplayProcessInfoForInjection();
			DisplayAttachedProcessInUI();
		}


		private void LoadDefaultNamesFromConfigFile()
		{
			_defaultDllName = ConfigurationManager.AppSettings["defaultDllName"] ?? string.Empty;
			_defaultProcessName = (ConfigurationManager.AppSettings["defaultProcessName"] ?? string.Empty).ToLowerInvariant();
			if(string.IsNullOrWhiteSpace(_defaultDllName))
			{
				return;
			}
			if(Path.GetExtension(_defaultDllName).ToLowerInvariant()!=".dll")
			{
				_defaultDllName = string.Empty;
				return;
			}
			// check if the default dll name contains a path. 
			var pathInDllName = Path.GetDirectoryName(_defaultDllName);
			if(pathInDllName==null)
			{
				// invalid
				_defaultDllName = string.Empty;
				return;
			}
			if(string.IsNullOrEmpty(pathInDllName))
			{
				_defaultDllName = Path.Combine(Environment.CurrentDirectory, _defaultDllName);
			}
			if(!File.Exists(_defaultDllName))
			{
				_defaultDllName = string.Empty;
				return;
			}
			// all clear. 
			_dllFilenameTextBox.Text = _defaultDllName;
		}
		

		private void FindDefaultProcess()
		{
			if(string.IsNullOrWhiteSpace(_defaultProcessName))
			{
				return;
			}
			var currentProcess = Process.GetCurrentProcess();
			_selectedProcess = null;
			foreach(var p in Process.GetProcesses())
			{
				try
				{
					if(p.SessionId == currentProcess.SessionId && !string.IsNullOrEmpty(p.MainWindowTitle) && p.MainModule.ModuleName.ToLowerInvariant() == _defaultProcessName)
					{
						_selectedProcess = p;
						break;
					}
				}
				catch
				{
					// security issues can occur, in which case we'll ignore the process. 
				}
			}
		}
		

		private string GetAbsolutePathForDllName()
		{
			var toReturn = _dllFilenameTextBox.Text;
			if(string.IsNullOrWhiteSpace(toReturn))
			{
				return string.Empty;
			}
			if (Path.IsPathRooted(toReturn))
			{
				return toReturn;
			}
			var rawToReturn = Path.Combine(Environment.CurrentDirectory, toReturn);
			return Path.GetFullPath(rawToReturn).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		}


		private void DisplayProcessInfoForInjection()
		{
			_processNameTextBox.Text = _selectedProcess==null ? "Please select a process..." 
															  : string.Format("({0}) {1} ({2})", _selectedProcess.Id, _selectedProcess.MainModule.ModuleName, _selectedProcess.MainWindowTitle);
		}
		

		private void DisplayAttachedProcessInUI()
		{
			var attachedProcess = AppStateSingleton.Instance().AttachedProcess;
			if(attachedProcess == null)
			{
				_injectDllGroupBox.Visibility = Visibility.Visible;
				_attachedProcessInfoGroupBox.Visibility = Visibility.Hidden;
				return;
			}
			_injectDllGroupBox.Visibility = Visibility.Collapsed;
			_attachedProcessInfoGroupBox.Visibility = Visibility.Visible;
			_executableIconImage.Source = System.Drawing.Icon.ExtractAssociatedIcon(attachedProcess.MainModule.FileName).ToBitmapImage();
			_executableTextBox.Text = attachedProcess.MainModule.FileName;
			_windowTitleTextBox.Text = attachedProcess.MainWindowTitle;
		}
		

		private void EnableDisableInjectButton()
		{
			_injectButton.IsEnabled = IsReadyToInject();
		}
				

		private bool IsReadyToInject()
		{
			var selectedFileExtension = (string.IsNullOrWhiteSpace(_dllFilenameTextBox.Text) ? string.Empty : Path.GetExtension(_dllFilenameTextBox.Text)) ?? string.Empty;
			return (_selectedProcess!=null && File.Exists(_dllFilenameTextBox.Text) && selectedFileExtension.ToLowerInvariant()==".dll");
		}
		

		private void _processNameTextBox_OnTextChanged(object sender, TextChangedEventArgs e)
		{
			EnableDisableInjectButton();
		}


		private void _dllFilenameTextBox_OnTextChanged(object sender, TextChangedEventArgs e)
		{
			EnableDisableInjectButton();
		}


		private void _injectButton_Click(object sender, EventArgs e)
		{
			if(!IsReadyToInject())
			{
				return;
			}
			// assume things are OK
			var injector = new DllInjector();
			string filenameOfDllToInject = GetAbsolutePathForDllName();
			var result = injector.PerformInjection(_selectedProcess.Id, filenameOfDllToInject);
			if(result)
			{
				// store dll with process name in recent list
				AppStateSingleton.Instance().AddDllNameForProcess(_selectedProcess.MainModule.ModuleName, filenameOfDllToInject);
				AppStateSingleton.Instance().SetAttachedProcess(_selectedProcess);
				_selectedProcess.EnableRaisingEvents = true;
				_selectedProcess.Exited += _selectedProcess_Exited;
				DisplayAttachedProcessInUI();
				this.DllInjected.RaiseEvent(this);
			}
			else
			{
				MessageBox.Show(string.Format("Injection failed when performing:{0}{1}{0}The following error occurred:{0}{2}",
													Environment.NewLine, injector.LastActionPerformed, new Win32Exception(injector.LastError).Message), "Injection result",
								MessageBoxButton.OK, MessageBoxImage.Error);
			}
		}


		private void _selectedProcess_Exited(object sender, EventArgs e)
		{
			// process has died, we should too. So sad.
			this.AttachedProcessExited.RaiseEvent(this);
		}


		private void _rehookXInputButton_OnClick(object sender, RoutedEventArgs e)
		{
			MessageHandlerSingleton.Instance().SendRehookXInputAction();
		}
	}
}
