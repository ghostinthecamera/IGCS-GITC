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
using ToastNotifications;

namespace IGCSClient.Classes
{
	/// <summary>
	/// Various extension methods used in the project
	/// </summary>
	public static class ExtensionMethods
	{
		/// <summary>
		/// Shows a custom message using the toastnotifications system.
		/// </summary>
		/// <param name="notifier"></param>
		/// <param name="message"></param>
		public static void ShowCustomMessage(this Notifier notifier, string message)
		{
			notifier.Notify<CustomNotification>(() => new CustomNotification(message));
		}


		/// <summary>
		/// Raises the event which is represented by the handler specified. 
		/// </summary>
		/// <typeparam name="T">type of the event args</typeparam>
		/// <param name="handler">The handler of the event to raise.</param>
		/// <param name="sender">The sender of the event.</param>
		/// <param name="arguments">The arguments to pass to the handler.</param>
		public static void RaiseEvent<T>(this EventHandler<T> handler, object sender, T arguments)
			where T : System.EventArgs
		{
			handler?.Invoke(sender, arguments);
		}
		

		/// <summary>
		/// Raises the event on the handler passed in with default empty arguments
		/// </summary>
		/// <param name="handler">The handler.</param>
		/// <param name="sender">The sender.</param>
		public static void RaiseEvent(this EventHandler handler, object sender)
		{
			handler?.Invoke(sender, EventArgs.Empty);
		}
	}
}
