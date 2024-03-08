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
using System.Windows;
using IGCSClient.Forms;
using IGCSClient.GameSpecific.Classes;

namespace IGCSClient
{
	/// App starting point
	/// </summary>
	public partial class App : Application
	{
		private static MainWindow _mainWindow;

		private void App_OnStartup(object sender, StartupEventArgs e)
		{
			Application.Current.DispatcherUnhandledException += Application_DispatcherUnhandledException;
			_mainWindow = new MainWindow();
			Init();
			_mainWindow.Show();
		}

		
		private static void Init()
		{
			SettingFactory.InitializeSettings();
			SettingFactory.InitializeKeyBindings();
		}


		private void Application_DispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
		{
			e.Handled = true;
			if(_mainWindow == null || _mainWindow.Dispatcher==null)
			{
				SimpleExceptionViewerWPF.Show(null, e.Exception, "An unhandled exception occurred.");
			}
			else
			{
				_mainWindow.Dispatcher?.Invoke(new Action<Exception>((a) => SimpleExceptionViewerWPF.Show(_mainWindow, a, "An Unhandled exception occurred.")), e.Exception);
			}
		}
	}
}
