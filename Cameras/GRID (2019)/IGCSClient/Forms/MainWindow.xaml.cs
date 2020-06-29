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
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using IGCSClient.Classes;
using IGCSClient.GameSpecific.Classes;
using ModernWpf.Controls;
using ModernWpf.Controls.Primitives;
using ToastNotifications;
using ToastNotifications.Lifetime;
using ToastNotifications.Position;

namespace IGCSClient.Forms
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		private Notifier _notificationNotifier;

		public MainWindow()
		{
			InitializeComponent();
			LogHandlerSingleton.Instance().Setup(_logControl);
			TabItemHelper.SetIcon(_generalTab, new SymbolIcon(Symbol.Repair));
			TabItemHelper.SetIcon(_hotSamplingTab, new SymbolIcon(Symbol.FullScreen));
			TabItemHelper.SetIcon(_configurationTab, new SymbolIcon(Symbol.Setting));
			TabItemHelper.SetIcon(_themeTab, new SymbolIcon(Symbol.Highlight));
			TabItemHelper.SetIcon(_keybindingsTab, new SymbolIcon(Symbol.Keyboard));
			TabItemHelper.SetIcon(_logTab, new SymbolIcon(Symbol.OpenFile));
			TabItemHelper.SetIcon(_aboutTab, new SymbolIcon(Symbol.People));
			TabItemHelper.SetIcon(_helpTab, new SymbolIcon(Symbol.Help));
			this.MinHeight = this.Height;
			this.MinWidth = this.Width;
		}


		protected override void OnClosing(CancelEventArgs e)
		{
			// first make sure the recently used resolutions are persisted
			AppStateSingleton.Instance().StoreRecentlyUsedResolutions(_hotsamplingControl.RecentlyUserResolutions);
			AppStateSingleton.Instance().SaveSettingsToIni();
			AppStateSingleton.Instance().SaveRecentProcessList();
			_notificationNotifier.Dispose();
			base.OnClosing(e);
		}


		private void MainWindow_OnLoaded(object sender, RoutedEventArgs e)
		{
			_sbHyperlink.NavigateUri = new Uri(ConstantsEnums.IGCSRootURL);
			_notificationNotifier = new Notifier(cfg =>
												 {
													 cfg.PositionProvider = new PrimaryScreenPositionProvider(corner: Corner.TopLeft, offsetX: 10, offsetY: 10);
													 cfg.LifetimeSupervisor = new TimeAndCountBasedLifetimeSupervisor(notificationLifetime: TimeSpan.FromSeconds(3),
																													  maximumNotificationCount: MaximumNotificationCount.FromCount(6));
													 cfg.Dispatcher = Application.Current.Dispatcher;
													 cfg.DisplayOptions.TopMost = true;
												 });
			this.Title = string.Format("{0} (v{1}). By {2}", GameSpecificConstants.ClientWindowTitle, GameSpecificConstants.CameraVersion, GameSpecificConstants.CameraCredits);

			// First setup the controls 
			_configurationEditor.Setup();
			_keyBindingsEditor.Setup();

			// then load the values from the ini file (if any) so the controls are already there.
			AppStateSingleton.Instance().LoadFromIni();
			AppStateSingleton.Instance().LoadRecentProcessList();
			MessageHandlerSingleton.Instance().ConnectedToNamedPipeFunc = () => HandleConnectedToPipe();
			MessageHandlerSingleton.Instance().ClientConnectionReceivedFunc = () => HandleConnectionReceived();

			// Disable all tabs, except general, log and about.
			_hotSamplingTab.IsEnabled = false;
			_configurationTab.IsEnabled = false;
			_keybindingsTab.IsEnabled = false;

			MessageHandlerSingleton.Instance().NotificationLogFunc = s => DisplayNotification(s);
		}


		private void DisplayNotification(string notification)
		{
			_notificationNotifier.ShowCustomMessage(notification);
		}


		private void HandleDllInjected()
		{
			// enable all tabs
			_hotSamplingTab.IsEnabled = true;
			_configurationTab.IsEnabled = true;
			_keybindingsTab.IsEnabled = true;
			// show the resolutions on the hotsampling tab
			_hotsamplingControl.BindData();
		}


		private void HandleConnectionReceived()
		{
			LogHandlerSingleton.Instance().LogLine("DLL Connected to our named pipe.", "System", true);
			SetTextOnTextBlock(_clientToDllConnectedSBLabel, "Client->Camera dll active");
		}

		
		private void HandleConnectedToPipe()
		{
			LogHandlerSingleton.Instance().LogLine("Connected to the DLL's named pipe", "System", true);
			if(this.CheckAccess())
			{
				AppStateSingleton.Instance().SendInitialData();
			}
			else
			{
				this.Dispatcher?.Invoke(()=>AppStateSingleton.Instance().SendInitialData());
			}
			LogHandlerSingleton.Instance().LogLine("Initial settings sent", "System", true);
			SetTextOnTextBlock(_dllToClientConnectedSBLabel, "Camera dll->client active");
		}


		private void SetTextOnTextBlock(TextBlock toSet, string toPass)
		{
			if(toSet == null)
			{
				return;
			}
			if(toSet.CheckAccess())
			{
				toSet.Text = toPass;
			}
			else
			{
				toSet.Dispatcher?.Invoke(()=>toSet.Text = toPass);
			}
		}


		private void _sbHyperlink_OnRequestNavigate(object sender, RequestNavigateEventArgs e)
		{
			Process.Start(e.Uri.ToString());
		}


		private void _generalTabControl_OnDllInjected(object sender, EventArgs e)
		{
			HandleDllInjected();
		}


		private void _generalTabControl_OnAttachedProcessExited(object sender, EventArgs e)
		{
			// Attached process died, we should too. So sad...
			if(this.CheckAccess())
			{
				this.Close();
			}
			else
			{
				this.Dispatcher?.Invoke(()=>this.Close());
			}
		}
	}
}

