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

namespace IGCSClient.Forms
{
	/// <summary>
	/// Generic exception viewer
	/// </summary>
	public partial class ExceptionViewerWPF : Window
	{
		#region Members
		private Exception _exception;
		#endregion

		public ExceptionViewerWPF()
		{
			InitializeComponent();
		}

		public ExceptionViewerWPF(Exception exceptionToView)
		{
			InitializeComponent();
			_exception = exceptionToView;
			ViewExceptionInWindow();
		}


		private void ViewExceptionInWindow()
		{
			if(_exception==null)
			{
				// no exception loaded
				_messageTextBox.Text = "No exception loaded into window: can't visualize exception";
				return;
			}
			// visualize the exception.
			_messageTextBox.Text+=_exception.Message + Environment.NewLine;
			_stackTraceTextBox.Text+="-----[Core exception]--------------------" + Environment.NewLine;
			_stackTraceTextBox.Text+= _exception.StackTrace + Environment.NewLine;
			// go into the inner exceptions.
			for(Exception innerException = _exception.InnerException;
				innerException!=null;innerException = innerException.InnerException)
			{
				_messageTextBox.Text += innerException.Message + Environment.NewLine;
				_stackTraceTextBox.Text += "-----[InnerException]--------------------" + Environment.NewLine;
				_stackTraceTextBox.Text += innerException.StackTrace + Environment.NewLine;
			}
			// strip extra newline from boxes
			_messageTextBox.Text = _messageTextBox.Text.Substring(0,_messageTextBox.Text.Length-1);
			_stackTraceTextBox.Text = _stackTraceTextBox.Text.Substring(0,_stackTraceTextBox.Text.Length-1);
		}


		private void _closeButton_OnClick(object sender, RoutedEventArgs e)
		{
			this.Close();
		}
	}
}
