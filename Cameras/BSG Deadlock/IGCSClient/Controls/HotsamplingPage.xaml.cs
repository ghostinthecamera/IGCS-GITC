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
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Forms;
using System.Windows.Input;
using IGCSClient.Classes;
using IGCSClient.GameSpecific.Classes;
using ModernWpf.Controls;
using DataObject = System.Windows.DataObject;
using UserControl = System.Windows.Controls.UserControl;

namespace IGCSClient.Controls
{
	/// <summary>
	/// Interaction logic for HotsamplingPage.xaml
	/// </summary>
	public partial class HotsamplingPage : UserControl
	{
		#region Members
		private WINDOWINFO _gameWindowInfo;
		private IntPtr _gameWindowHwnd;
		private Timer _resolutionRefreshTimer;
		private Resolution _monitorResolution;
		#endregion


		public HotsamplingPage()
		{
			InitializeComponent();
			_resolutionRefreshTimer = new Timer() { Interval = 1000};
			_resolutionRefreshTimer.Tick += _resolutionRefreshTimer_Tick;
			DataObject.AddCopyingHandler(_newHeightBox, (s, e) => { if (e.IsDragDrop) e.CancelCommand(); });
			DataObject.AddCopyingHandler(_newWidthBox, (s, e) => { if (e.IsDragDrop) e.CancelCommand(); });
		}


		public void BindData()
		{
			GetActiveGameWindowInfo();
			// game window hwnd is set in getactivegamewindowinfo. 
			// Use Windows Forms. This app is opt-in on high dpi awareness, so the values should be correct. This high-dpi awareness will fuck up winforms controls but we're
			// not using these so it's ok. It's otherwise a PITA to get the monitor resolution of the monitor the window is on!
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			BuildResolutionTree(screenWithGameWindow.Bounds);
			_resolutionRefreshTimer.Enabled = true;
			this.RecentlyUserResolutions = new ObservableCollection<Resolution>(AppStateSingleton.Instance().RecentlyUsedResolutions);
			_recentlyUsedResolutionsList.ItemsSource = this.RecentlyUserResolutions;
		}
		

		private void BuildResolutionTree(System.Drawing.Rectangle screenBounds)
		{
			var arOfScreen = GeneralUtils.CalculateAspectRatio(screenBounds.Width, screenBounds.Height);
			arOfScreen.Description = "Monitor aspect ratio";
			_monitorResolution = new Resolution(arOfScreen, screenBounds.Width, screenBounds.Height, 
												string.Format("{0} x {1} ({2})", screenBounds.Width, screenBounds.Height, arOfScreen.ToString(false)));
			// default ARs
			var defaultARs = new List<AspectRatio>()
							 {
								 new AspectRatio(16, 9), new AspectRatio(16, 10), new AspectRatio(21, 9), new AspectRatio(1, 1),
								 new AspectRatio(9, 16), new AspectRatio(2, 3), new AspectRatio(3, 2), new AspectRatio(2, 1), new AspectRatio(3, 4), new AspectRatio(4, 5)
							 };
			// remove the one we already have determined from the monitor (if present)
			defaultARs.Remove(arOfScreen);
			var resolutions = new List<Resolution>();
			// first the resolutions based on the screen resolution
			AddResolutionsForAspectRatio(resolutions, screenBounds.Width, screenBounds.Height, arOfScreen);
			// then the resolutions calculated from the aspect ratios in the list. 
			foreach(var ar in defaultARs)
			{
				if(ar.Horizontal <= ar.Vertical)
				{
					// use height instead of width so the full shot can be setup on the lowest res
					AddResolutionsForAspectRatio(resolutions, (int)(screenBounds.Height * ar.ARFactorWidth), screenBounds.Height, ar);
				}
				else
				{
					AddResolutionsForAspectRatio(resolutions, screenBounds.Width, (int)(screenBounds.Width * ar.ARFactorHeight), ar);
				}
			}

			_resolutionTreeView.ItemsSource = resolutions.GroupBy(r => r.AR.ToString());
		}


		private void AddResolutionsForAspectRatio(List<Resolution> resolutions, int initialWidth, int initialHeight, AspectRatio ar)
		{
			for(decimal i = 1; i < 6; i++)
			{
				resolutions.Add(GetResolutionForAR(initialWidth, initialHeight, ar, i));
				resolutions.Add(GetResolutionForAR(initialWidth, initialHeight, ar, i + 0.25m));
				resolutions.Add(GetResolutionForAR(initialWidth, initialHeight, ar, i + 0.5m));
				resolutions.Add(GetResolutionForAR(initialWidth, initialHeight, ar, i + 0.75m));
			}
		}


		private Resolution GetResolutionForAR(int width, int height, AspectRatio ar, decimal factor)
		{
			int newWidth = (int)(width * factor);
			int newHeight = (int)(height * factor);
			return new Resolution(ar, newWidth, newHeight, string.Format("{0} x {1} ({2:##.##}x)", newWidth, newHeight, factor));
		}


		private void PerformHotSampling()
		{
			if(_newWidthBox.Value < 100 || _newHeightBox.Value < 100)
			{
				return;
			}
			if((Win32Wrapper.IsIconic(_gameWindowHwnd) || Win32Wrapper.IsZoomed(_gameWindowHwnd)))
			{
				Win32Wrapper.ShowWindow(_gameWindowHwnd, Win32Wrapper.SW_SHOWNOACTIVATE);
			}

			int newHorizontalResolution = (int)_newWidthBox.Value;
			int newVerticalResolution = (int)_newHeightBox.Value;
			// always set the window at (0,0), if width is >= screen width.
			var screenBounds = Screen.FromHandle(_gameWindowHwnd).Bounds;
			uint uFlags = Win32Wrapper.SWP_NOZORDER | Win32Wrapper.SWP_NOACTIVATE | Win32Wrapper.SWP_NOOWNERZORDER | Win32Wrapper.SWP_NOSENDCHANGING;
			if(newHorizontalResolution < screenBounds.Width)
			{
				// let the window be where it is.
				uFlags |= Win32Wrapper.SWP_NOMOVE;
			}

			Win32Wrapper.SetWindowPos(_gameWindowHwnd, 0, 0, 0, newHorizontalResolution, newVerticalResolution, uFlags);
			if(GameSpecificConstants.HotsamplingRequiresEXITSIZEMOVE)
			{
				// A warning of unreachable code will be raised here, that's ok. this code is only used when the constant is true
				Win32Wrapper.SendMessage(_gameWindowHwnd, Win32Wrapper.WM_EXITSIZEMOVE, 0, 0);
			}

			LogHandlerSingleton.Instance().LogLine("Switching resolution on window with hwnd 0x{0} to resolution {1}x{2}", "Hotsampler", _gameWindowHwnd.ToString("x"), 
												   newHorizontalResolution, newVerticalResolution);

			// remove / add borders
			uint nStyle = (uint)Win32Wrapper.GetWindowLong(_gameWindowHwnd, Win32Wrapper.GWL_STYLE);
			nStyle = (nStyle | (Win32Wrapper.WS_THICKFRAME + Win32Wrapper.WS_DLGFRAME + Win32Wrapper.WS_BORDER + Win32Wrapper.WS_MAXIMIZEBOX + Win32Wrapper.WS_MINIMIZEBOX));
			if(_useWindowBordersCheckBox.IsChecked==false)
			{
				nStyle^=(Win32Wrapper.WS_THICKFRAME + Win32Wrapper.WS_DLGFRAME + Win32Wrapper.WS_BORDER + Win32Wrapper.WS_MAXIMIZEBOX + Win32Wrapper.WS_MINIMIZEBOX);
			}
			Win32Wrapper.SetWindowLong(_gameWindowHwnd, Win32Wrapper.GWL_STYLE, nStyle);
			uFlags = Win32Wrapper.SWP_NOSIZE | Win32Wrapper.SWP_NOMOVE | Win32Wrapper.SWP_NOZORDER | Win32Wrapper.SWP_NOACTIVATE | Win32Wrapper.SWP_NOOWNERZORDER | Win32Wrapper.SWP_NOSENDCHANGING | Win32Wrapper.SWP_FRAMECHANGED;
			Win32Wrapper.SetWindowPos(_gameWindowHwnd, 0, 0, 0, 0, 0, uFlags);

			AddResolutionToRecentlyUsedList(newHorizontalResolution, newVerticalResolution);

			if(_switchAfterResizingCheckBox.IsChecked==true)
			{
				// focus the attached application's window
				Win32Wrapper.SetForegroundWindow(_gameWindowHwnd);
			}
		}


		private void AddResolutionToRecentlyUsedList(int horizontalResolution, int verticalResolution)
		{
			// Base the aspect ratio on the string representation we have constructed. This is a bit lame, but it can be the user changes the width or height
			// manually after selecting a node in the tree so the ar has changed. Calculating it again from width/height is tempting but gives wrong results for 
			// 21:9 which isn't really 21:9 but 21.5:9 so the AR doesn't match anymore. 
			var ar = string.IsNullOrEmpty(_aspectRatioTextBox.Text) ? GeneralUtils.CalculateAspectRatio(horizontalResolution, verticalResolution) 
																	: AspectRatio.FromString(_aspectRatioTextBox.Text);
			var toAdd = new Resolution(ar, horizontalResolution, verticalResolution, string.Format("{0} x {1} ({2})", horizontalResolution, verticalResolution, ar));
			this.RecentlyUserResolutions.Remove(toAdd);	// if we've used this one before, remove it from the list as it will be placed at the front.
			this.RecentlyUserResolutions.Insert(0, toAdd);
			// remove any resolutions over the maximum. This is at most 1, as we're adding 1 at a time. 
			while(this.RecentlyUserResolutions.Count > ConstantsEnums.NumberOfResolutionsToKeep)
			{
				this.RecentlyUserResolutions.RemoveAt(ConstantsEnums.NumberOfResolutionsToKeep);
			}
		}


		private void GetActiveGameWindowInfo()
		{
			_gameWindowHwnd = AppStateSingleton.Instance().GetMainWindowHandleOfAttachedProcess();
			if(_gameWindowHwnd == IntPtr.Zero)
			{
				_currentWidthTextBox.Text = string.Empty;
				_currentHeightTextBox.Text = string.Empty;
				_currentARTextBox.Text = string.Empty;
				return;
			}
			Win32Wrapper.GetWindowInfo(_gameWindowHwnd, ref _gameWindowInfo);
			_currentWidthTextBox.Text = _gameWindowInfo.rcWindow.Width.ToString();
			_currentHeightTextBox.Text = _gameWindowInfo.rcWindow.Height.ToString();
			_currentARTextBox.Text = GeneralUtils.CalculateAspectRatio(_gameWindowInfo.rcWindow.Width, _gameWindowInfo.rcWindow.Height).ToString(appendDescription:false);
			if(_newHeightBox.Value <= 0 || _newWidthBox.Value <= 0)
			{
				//reset with current window
				_newHeightBox.Value = _gameWindowInfo.rcWindow.Height;
				_newWidthBox.Value = _gameWindowInfo.rcWindow.Width;
				_aspectRatioTextBox.Text = _currentARTextBox.Text;
			}
		}
		

		private void HandleResolutionValueChanged()
		{
			int horizontalResolution = (int)_newWidthBox.Value;
			int verticalResolution = (int)_newHeightBox.Value;
			_setResolutionButton.IsEnabled = (horizontalResolution >= 640 && verticalResolution >= 480);
			_aspectRatioTextBox.Text = GeneralUtils.CalculateAspectRatio(horizontalResolution, verticalResolution).ToString(appendDescription:false);
		}
				

		private void RepositionWindow(int newX, int newY)
		{
			if(_gameWindowHwnd == IntPtr.Zero)
			{
				return;
			}
			if((Win32Wrapper.IsIconic(_gameWindowHwnd) || Win32Wrapper.IsZoomed(_gameWindowHwnd)))
			{
				Win32Wrapper.ShowWindow(_gameWindowHwnd, Win32Wrapper.SW_SHOWNOACTIVATE);
			}
			uint uFlags = Win32Wrapper.SWP_NOSIZE | Win32Wrapper.SWP_NOZORDER | Win32Wrapper.SWP_NOACTIVATE | Win32Wrapper.SWP_NOOWNERZORDER | Win32Wrapper.SWP_NOSENDCHANGING | Win32Wrapper.SWP_FRAMECHANGED;
			Win32Wrapper.SetWindowPos(_gameWindowHwnd, 0, newX, newY, 0, 0, uFlags);
		}


		private void _resolutionTreeView_OnSelectedItemChanged(object sender, RoutedPropertyChangedEventArgs<object> e)
		{
			if(!(e.NewValue is Resolution))
			{
				// set to disabled. 
				return;
			}
			var selectedResolution = (Resolution)e.NewValue;
			SetNewResolutionControlsWithValues(selectedResolution);
		}


		private void SetNewResolutionControlsWithValues(Resolution selectedResolution)
		{
			_newHeightBox.Value = selectedResolution.VerticalResolution;
			_newWidthBox.Value = selectedResolution.HorizontalResolution;
			_aspectRatioTextBox.Text = selectedResolution.AR.ToString(appendDescription: false);
		}


		private void _setResolutionButton_OnClick(object sender, RoutedEventArgs e)
		{
			PerformHotSampling();
		}


		private void _newWidthBox_OnValueChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
		{
			_newWidthBox.Value = GeneralUtils.ClampNaN(args.OldValue, args.NewValue);
			HandleResolutionValueChanged();
		}


		private void _newHeightBox_OnValueChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
		{
			_newHeightBox.Value = GeneralUtils.ClampNaN(args.OldValue, args.NewValue);
			HandleResolutionValueChanged();
		}

		
		private void _resolutionRefreshTimer_Tick(object sender, EventArgs e)
		{
			GetActiveGameWindowInfo();
		}


		private void _switchToGameWindowButton_OnClick(object sender, RoutedEventArgs e)
		{
			// focus the attached application's window
			Win32Wrapper.SetForegroundWindow(_gameWindowHwnd);
		}


		private void _refreshButton_OnClick(object sender, RoutedEventArgs e)
		{
			GetActiveGameWindowInfo();
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			BuildResolutionTree(screenWithGameWindow.Bounds);
		}


		private void _recentlyUsedResolutionsList_OnPreviewMouseDown(object sender, MouseButtonEventArgs e)
		{
			var item = ItemsControl.ContainerFromElement(_recentlyUsedResolutionsList, e.OriginalSource as DependencyObject) as ListBoxItem;
			if (item != null)
			{
				SetNewResolutionControlsWithValues((Resolution)item.Content);
				_recentlyUsedResolutionsFlyout.Hide();
				PerformHotSampling();
			}
		}
		

		private void _fakeFullScreen_OnClick(object sender, RoutedEventArgs e)
		{
			SetNewResolutionControlsWithValues(_monitorResolution);
			PerformHotSampling();
		}
		
		
		private void _leftAlignButton_Click(object sender, RoutedEventArgs e)
		{
			RepositionWindow(0, _gameWindowInfo.rcWindow.top);
		}


		private void _rightAlignButton_Click(object sender, RoutedEventArgs e)
		{
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			RepositionWindow(screenWithGameWindow.Bounds.Width-_gameWindowInfo.rcWindow.Width, _gameWindowInfo.rcWindow.top);
		}


		private void _topAlignButton_Click(object sender, RoutedEventArgs e)
		{
			RepositionWindow(_gameWindowInfo.rcWindow.left, 0);
		}


		private void _bottomAlignButton_Click(object sender, RoutedEventArgs e)
		{
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			RepositionWindow(_gameWindowInfo.rcWindow.left, screenWithGameWindow.Bounds.Height-_gameWindowInfo.rcWindow.Height);
		}


		private void _horizontalCenterAlignButton_Click(object sender, RoutedEventArgs e)
		{
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			RepositionWindow((screenWithGameWindow.Bounds.Width-_gameWindowInfo.rcWindow.Width)/2, _gameWindowInfo.rcWindow.top);
		}


		private void _verticalCenterAlignButton_Click(object sender, RoutedEventArgs e)
		{
			var screenWithGameWindow = Screen.FromHandle(_gameWindowHwnd);
			RepositionWindow(_gameWindowInfo.rcWindow.left, (screenWithGameWindow.Bounds.Height-_gameWindowInfo.rcWindow.Height)/2);
		}


		#region Properties
		public ObservableCollection<Resolution> RecentlyUserResolutions { get; set; }
		#endregion
	}
}
