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
using IGCSClient.Classes;

namespace IGCSClient.NamedPipeSubSystem
{
	/// <summary>
	/// Base class for named pipe stream oriented classes
	/// </summary>
	/// <remarks>Roughly based on Peter Bromberg's AsyncPipes library.</remarks>
	public abstract class NamedPipeStreamBase : IDisposable
	{
		#region Class Member Declarations

		private readonly object _eventLock = new object();
		private EventHandler<ContainerEventArgs<byte[]>> _messageReceived;
		private readonly string _pipeName;
		#endregion

		#region Events
		/// <summary>
		/// Event which is raised when a new message has been received
		/// </summary>
		public event EventHandler<ContainerEventArgs<byte[]>> MessageReceived
		{
			add
			{
				lock(this._eventLock)
				{
					_messageReceived = (EventHandler<ContainerEventArgs<byte[]>>)Delegate.Combine(_messageReceived, value);
				}
			}
			remove
			{
				lock(this._eventLock)
				{
					_messageReceived = (EventHandler<ContainerEventArgs<byte[]>>)Delegate.Remove(_messageReceived, value);
				}
			}
		}
		#endregion


		/// <summary>
		/// Initializes a new instance of the <see cref="NamedPipeStreamBase"/> class.
		/// </summary>
		/// <param name="pipeName">Name of the pipe.</param>
		protected NamedPipeStreamBase(string pipeName)
		{
			_pipeName = pipeName;
		}


		/// <summary>
		/// Releases unmanaged resources and performs other cleanup operations before the
		/// <see cref="NamedPipeStreamBase"/> is reclaimed by garbage collection.
		/// </summary>
		~NamedPipeStreamBase()
		{
			this.Dispose(false);
		}


		/// <summary>
		/// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
		/// </summary>
		public void Dispose()
		{
			this.Dispose(true);
		}


		/// <summary>
		/// Disconnects this instance.
		/// </summary>
		public virtual void Disconnect()
		{
			lock(_eventLock)
			{
				_messageReceived = null;
			}
		}


		/// <summary>
		/// Releases unmanaged and - optionally - managed resources
		/// </summary>
		/// <param name="disposing"><c>true</c> to release both managed and unmanaged resources; <c>false</c> to release only unmanaged resources.</param>
		protected virtual void Dispose(bool disposing)
		{
			if (disposing)
			{
				GC.SuppressFinalize(this);
			}
		}


		/// <summary>
		/// Raises the <see cref="E:MessageReceived"/> event.
		/// </summary>
		/// <param name="args">The <see cref="SD.Tools.Algorithmia.GeneralDataStructures.EventArguments.ContainerEventArgs&lt;System.Byte[]&gt;"/> instance containing the event data.</param>
		protected virtual void OnMessageReceived(ContainerEventArgs<byte[]> args)
		{
			EventHandler<ContainerEventArgs<byte[]>> handlers;
			lock (this._eventLock)
			{
				handlers = _messageReceived;
			}
			handlers.RaiseEvent(this, args);
		}


		#region Class Property Declarations
		/// <summary>
		/// Gets the name of the pipe.
		/// </summary>
		protected string PipeName
		{
			get { return _pipeName; }
		}
		#endregion
	}
}
